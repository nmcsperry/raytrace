#include <stdio.h>
#include <math.h>

#include "window_stuff.c"

#define DEBUG_PRINT(...) { \
    char _debug_str[256]; \
    sprintf(_debug_str, __VA_ARGS__); \
    OutputDebugString(_debug_str); \
}

#define ARRAY_LEN(x) sizeof((x))/sizeof((x)[0])

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Color {
    float r;
    float g;
    float b;
} Color;

typedef struct Material {
    Color color;
    float mirror;
} Material;

typedef struct Sphere {
    float r;
    Vector3 pos;
    Material material;
} Sphere;

typedef struct Plane {
    Vector3 pos;
    Vector3 normal;
    Material material;
} Plane;

typedef struct Checkerboard {
    Plane plane;
    Material material_2;
    float scale;
} Checkerboard;

typedef struct Ray {
    Vector3 start;
    Vector3 dir;
} Ray;

typedef struct Light {
    Vector3 pos;
    Color color;
} Light;

typedef enum Object_Type {
    OBJ_SPHERE, OBJ_PLANE, OBJ_CHECKERBOARD
} Object_Type;

typedef struct Object {
    Object_Type type;
    union {
        Sphere sphere;
        Plane plane;
        Checkerboard checkerboard;
    };
} Object;

bool fequal (float a, float b) {
    float epsilon = 0.00001;
    if (a + epsilon > b && a - epsilon < b) return true;
    return false;
}

int quadform (float a, float b, float c, float *answers) {
    if (b*b - 4.0*a*c < 0.0) return 0;

    if (fequal(b*b - 4.0*a*c, 0.0f)) {
        float x1 = -b / 2.0f / a;
        answers[0] = x1;
        return 1;
    }

    float x1 = (-b + sqrt(b*b - 4.0*a*c)) / 2.0f / a;
    float x2 = (-b - sqrt(b*b - 4.0*a*c)) / 2.0f / a;

    answers[0] = x1;
    answers[1] = x2;

    return 2;
}

int quadform_only_positive (float a, float b, float c, float *answers) {
    if (b*b - 4.0*a*c < 0.0) return 0;

    if (fequal(b*b - 4.0*a*c, 0.0f)) {
        float x1 = -b / 2.0f / a;

        if (x1 > 0) {
            answers[0] = x1;
            return 1;
        } else {
            return 0;
        }
    }

    float x1 = (-b + sqrt(b*b - 4.0*a*c)) / 2.0f / a;
    float x2 = (-b - sqrt(b*b - 4.0*a*c)) / 2.0f / a;

    int index = 0;

    if (x1 >= 0) {
        answers[index] = x1;
        index++;
    }

    if (x2 >= 0) {
        answers[index] = x2;
        index++;
    }

    return index;
}

float sq (float a) {
    return a*a;
}

Vector3 vec3_add (Vector3 a, Vector3 b) {
    Vector3 sum = {a.x + b.x, a.y + b.y, a.z + b.z};
    return sum;
}

float fclamp (float a, float max, float min) {
    if (a > max) return max;
    if (a < min) return min;
    return a;
}

Color color_add (Color a, Color b) {
    Color sum = {
        fclamp(a.r + b.r, 1.0f, 0.0f),
        fclamp(a.g + b.g, 1.0f, 0.0f),
        fclamp(a.b + b.b, 1.0f, 0.0f)
    };
    return sum;
}

Vector3 vec3_sub (Vector3 a, Vector3 b) {
    Vector3 diff = {a.x - b.x, a.y - b.y, a.z - b.z};
    return diff;
}

float vec3_dot (Vector3 a, Vector3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vector3 vec3_div (Vector3 a, float b) {
    Vector3 vec = {a.x / b, a.y / b, a.z / b};
    return vec;
}

Vector3 vec3_mul (Vector3 a, float b) {
    Vector3 vec = {a.x * b, a.y * b, a.z * b};
    return vec;
}

Vector3 vec3_normalize (Vector3 a) {
    float norm = sqrt(vec3_dot(a, a));
    return vec3_div(a, norm);
}

Vector3 vec3_cross (Vector3 a, Vector3 b) {
    return (Vector3) {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x,
    };
}

Vector3 sphere_normal (Sphere sphere, Vector3 point) {
    return vec3_normalize(vec3_sub(point, sphere.pos));
}

Vector3 plane_normal (Plane plane, Vector3 point) {
    return vec3_normalize(plane.normal);
}

Vector3 object_normal (Object object, Vector3 point) {
    Vector3 normal = {0};
    switch (object.type) {
        case OBJ_SPHERE:
            normal = sphere_normal(object.sphere, point);
            break;
        case OBJ_PLANE:
            normal = plane_normal(object.plane, point);
            break;
        case OBJ_CHECKERBOARD:
            normal = plane_normal(object.checkerboard.plane, point);
            break;
    }

    return normal;
}

Vector3 parametric_line (float t, Ray ray) {
    Vector3 offset = vec3_mul(ray.dir, t);
    Vector3 point = vec3_add(ray.start, offset);
    return point;
}

bool intersect_plane (Ray ray, Plane plane, float *parametric_hit) {
    float plane_offset = vec3_dot(plane.normal, plane.pos);

    float x = (plane_offset - vec3_dot(plane.normal, ray.start))
        / vec3_dot(plane.normal, ray.dir);

    if (x > 0) {
        if (parametric_hit != NULL) *parametric_hit = x;
        return true;
    }

    return false;
}

bool intersect_sphere (Ray ray, Sphere sphere, float *parametric_hit) {
    Vector3 sphere_off = vec3_sub(ray.start, sphere.pos);

    float a = sq(ray.dir.x) + sq(ray.dir.y) + sq(ray.dir.z);
    float b = 2.0f*ray.dir.x*sphere_off.x + 2.0f*ray.dir.y*sphere_off.y +
        2.0f*ray.dir.z*sphere_off.z;
    float c = sq(sphere_off.x) + sq(sphere_off.y) + sq(sphere_off.z) - sq(sphere.r);

    float answers[2];

    int num_answers = quadform_only_positive(a, b, c, answers);

    switch (num_answers) {
        case 0:
            return false;

        case 1:
            if (parametric_hit != NULL)
                *parametric_hit = answers[0];
            return true;

        case 2:
            if (parametric_hit != NULL)
                if (answers[0] < answers[1])
                    *parametric_hit = answers[0];
                else
                    *parametric_hit = answers[1];
            return true;

        default:
            DEBUG_PRINT("Something is seriously messed up.");
            return false;
    }

    if (num_answers > 0) return true;
    return false;
}

Object scene[4];
Light lights[3];

void setup_scene () {
    scene[1] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {8.0f, 1.5f, 22.5f},
        .sphere.r = 3.0f,

        .sphere.material.color = (Color) {1.0f, 0.3f, 0.3f},
        .sphere.material.mirror = 0.0f
    };

    scene[2] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {0.0f, 3.0f, 25.0f},
        .sphere.r = 6.0f,

        .sphere.material.color = (Color) {0.3f, 0.3f, 1.0f},
        .sphere.material.mirror = 0.8f
    };

    scene[0] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {-9.0f, 1.2f, 25.0f},
        .sphere.r = 4.0f,

        .sphere.material.color = (Color) {0.3f, 1.0f, 0.3f},
        .sphere.material.mirror = 0.0f
    };

    // scene[4] = (Object) {
    //     .type = OBJ_SPHERE,

    //     .sphere.pos = (Vector3) {0.0f, 16.0f, 21.0f},
    //     .sphere.r = 4.0f,

    //     .sphere.material.color = (Color) {0.3f, 0.3f, 1.0f},
    //     .sphere.material.mirror = false
    // };

    scene[3] = (Object) {
        .type = OBJ_CHECKERBOARD,

        .checkerboard.plane.pos = (Vector3) {0.0f, 3.0f, 27.0f},
        .checkerboard.plane.normal = (Vector3) {-0.5f, 1.0f, -1.0f},

        .checkerboard.plane.material.color = (Color) {1.0f, 1.0f, 1.0f},
        .checkerboard.plane.material.mirror = 0.1f,

        .checkerboard.material_2.color = (Color) {0.3f, 0.3f, 0.3f},
        .checkerboard.material_2.mirror = 0.1f,

        .checkerboard.scale = 3.0f
    };
    scene[3].plane.normal = vec3_normalize(scene[3].plane.normal);

    lights[0] = (Light) {
        .color = (Color) {0.5f, 1.0f, 1.0f},
        .pos = (Vector3) {20.0f, 15.0f, 15.0f}
    };

    lights[1] = (Light) {
        .color = (Color) {0.7f, 0.7f, 0.5f},
        .pos = (Vector3) {0.0f, 0.0f, 5.0f}
    };

    // lights[2] = (Light) {
    //     .color = (Color) {0.7f, 0.7f, 0.5f},
    //     .pos = (Vector3) {0.0f, 20.0f, 21.0f},
    // };
}

Material checkerboard_choose_material (
    Checkerboard checkerboard, Vector3 point
) {
    Vector3 v0 = (Vector3) {1.0f, 0.0f, 0.0f};   
    Vector3 u = vec3_normalize(vec3_cross(checkerboard.plane.normal, v0));
    Vector3 v = vec3_normalize(vec3_cross(checkerboard.plane.normal, u));

    Vector3 ref = vec3_sub(point, checkerboard.plane.pos);

    int ui = (int) ceil(vec3_dot(u, ref) / checkerboard.scale);
    int vi = (int) ceil(vec3_dot(v, ref) / checkerboard.scale);

    if ((unsigned)ui % 2 != (unsigned)vi % 2)
        return checkerboard.plane.material;
    else
        return checkerboard.material_2;
}

Material object_material (Object object, Vector3 point) {
    Material material;
    switch (object.type) {
        case OBJ_SPHERE:
            material = object.sphere.material;
            break;
        case OBJ_PLANE:
            material = object.plane.material;
            break;
        case OBJ_CHECKERBOARD:
            material = checkerboard_choose_material(
                object.checkerboard, point);
            break;
    }

    return material;
}

bool intersect_scene (Ray ray, float *hit, int *hit_object) {
    float closest_hit = INFINITY;
    int closest_hit_object = -1;
    bool found_anything = false;

    for (int i = 0; i < ARRAY_LEN(scene); i++) {
        float this_hit;

        bool intersect = false;

        switch (scene[i].type) {
            case OBJ_SPHERE:
                intersect = intersect_sphere(ray, scene[i].sphere, &this_hit);
                break;
            case OBJ_PLANE:
                intersect = intersect_plane(ray, scene[i].plane, &this_hit);
                break;
            case OBJ_CHECKERBOARD:
                intersect = intersect_plane(ray,
                    scene[i].checkerboard.plane, &this_hit);
                break;
        }
        
        if (intersect) {
            found_anything = true;

            if (this_hit < closest_hit) {
                closest_hit = this_hit;
                closest_hit_object = i;
            }
        }
    }

    if (found_anything) {
        *hit = closest_hit;
        *hit_object = closest_hit_object;
    }

    return found_anything;
}

bool intersect_scene_with_one_exception (
    Ray ray, float *hit, int *hit_object, int exception
) {
    float closest_hit = INFINITY;
    int closest_hit_object = -1;
    bool found_anything = false;

    for (int i = 0; i < ARRAY_LEN(scene); i++) {
        if (i == exception) continue;

        float this_hit;

        bool intersect = false;

        switch (scene[i].type) {
            case OBJ_SPHERE:
                intersect = intersect_sphere(ray, scene[i].sphere, &this_hit);
                break;
            case OBJ_PLANE:
                intersect = intersect_plane(ray, scene[i].plane, &this_hit);
                break;
            case OBJ_CHECKERBOARD:
                intersect = intersect_plane(ray, scene[i].checkerboard.plane, &this_hit);
                break;
        }
        
        if (intersect) {
            found_anything = true;

            if (this_hit < closest_hit) {
                closest_hit = this_hit;
                closest_hit_object = i;
            }
        }
    }

    if (found_anything) {
        if (hit != NULL)
            *hit = closest_hit;
        if (hit_object != NULL)
            *hit_object = closest_hit_object;
    }

    return found_anything;
}

Color color_from_light (Light light, Object object, Vector3 point) {
    Color result = {0};
    
    Vector3 normal = object_normal(object, point);

    Vector3 direction_to_light = vec3_normalize(vec3_sub(
        point, light.pos));

    float lightness = -1.0f * vec3_dot(direction_to_light, normal);

    if (lightness > 0) {
        result.g = lightness * light.color.g;
        result.b = lightness * light.color.b;
        result.r = lightness * light.color.r;
    }

    return result;
}

Color color_mul (Color a, Color b) {
    Color product = {a.r * b.r, a.g * b.g, a.b * b.b};
    return product;
}

Color color_from_all_lights (int object_index, Vector3 point) {
    Color result = {0};

    Object object = scene[object_index];

    for (int i = 0; i < ARRAY_LEN(lights); i++) {
        Color this_color = color_from_light(lights[i], object, point);

        Ray shadow_ray = {0};
        shadow_ray.start = point;
        shadow_ray.dir = vec3_sub(lights[i].pos, point);

        bool did_we_hit = intersect_scene_with_one_exception(
            shadow_ray, NULL, NULL, object_index);
        
        if (!did_we_hit)
            result = color_add(result, this_color);
    }
    
    return color_mul(result, object_material(object, point).color);
}

Color color_lerp (Color a, Color b, float lerp) {
    return (Color) {
        (1.0f - lerp) * a.r + lerp * b.r,
        (1.0f - lerp) * a.g + lerp * b.g,
        (1.0f - lerp) * a.b + lerp * b.b,
    };
}

Color get_ray_color_with_one_exception (Ray sight, int depth, int exception) {
    Color result = {0};

    if (depth > 20) {
        result = (Color) {1.0f, 1.0f, 1.0f};
        return result;
    }

    float hit;
    int hit_object;
    
    if (intersect_scene_with_one_exception(sight, &hit, &hit_object, exception)) {
        Vector3 hit_point = parametric_line(hit, sight);

        float mirror = object_material(scene[hit_object], hit_point).mirror;

        if (mirror > 0.0f) {
            Vector3 normal = object_normal(scene[hit_object], hit_point);
            Vector3 dir = vec3_normalize(sight.dir);
        
            Vector3 reflection_dir = vec3_sub(dir, vec3_mul(normal, 2.0f * vec3_dot(dir, normal)));

            Ray reflection = {0};
            reflection.dir = vec3_normalize(reflection_dir);
            reflection.start = hit_point;

            Color mirror_color =
                get_ray_color_with_one_exception(reflection, depth + 1, hit_object);
            Color light_color = color_from_all_lights(hit_object, hit_point); 

            result = color_lerp(light_color, mirror_color, mirror);
        } else {
            result = color_from_all_lights(hit_object, hit_point); 
        }
    }

    return result;
}

void raytrace (Win32_Offscreen_Buffer *buffer) {
    u8 * row = (u8 *) buffer->memory;

    for (int y = 0; y < buffer->height; ++y) {
    
        u32 * pixel = (u32 *) row; 
        for (int x = 0; x < buffer->width; ++x) {

            Ray sight = {0};
            sight.dir.x = (-(float)x + buffer->width/2 ) / buffer->height;
            sight.dir.y = (-(float)y + buffer->height/2) / buffer->height;
            sight.dir.z = 1;

            Color surface_color = get_ray_color_with_one_exception(sight, 0, -1);

            u8 green = (u8)(surface_color.g * 255);
            u8 blue  = (u8)(surface_color.b * 255);
            u8 red   = (u8)(surface_color.r * 255);

            *pixel++ = ((red << 16) | (green << 8) | blue);
        }
        row += buffer->pitch;
    }
}

int CALLBACK WinMain (
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR command_line,
    int show_code
) {
    setup_scene();

    WNDCLASSA window_class = {0};

    int window_width = 1280;
    int window_height = 720;


    /*r*/window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "SineWaveWindow";

    RegisterClassA(&window_class);

    HWND window = CreateWindowExA(
        0,
        window_class.lpszClassName,
        "Ray Tracer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_width,
        window_height,
        0,
        0,
        instance,
        0
    );

    HDC device_context = GetDC(window);
    // this assumes we got something

    Win32_Window_Dimension starting_dim = win32_get_window_dimension(window);
    win32_resize_dib_section(
        &global_backbuffer, starting_dim.width, starting_dim.height
    );
    
    raytrace(&global_backbuffer);

    global_running = true;
    while(global_running) {

        MSG message;

        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
                global_running = 0.0f;

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        Win32_Window_Dimension dimension = win32_get_window_dimension(window);
        win32_display_buffer_in_window(
            device_context,
            dimension.width,
            dimension.height,
            &global_backbuffer,
            0, 0,
            dimension.width, dimension.height
        );

        //ReleaseDC(window, device_context);
    }

    return 0;
}
