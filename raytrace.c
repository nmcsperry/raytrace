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

typedef struct Sphere {
    float r;
    Vector3 pos;
} Sphere;

typedef struct Ray {
    Vector3 start;
    Vector3 dir;
} Ray;

typedef struct Color {
    float r;
    float g;
    float b;
} Color;

typedef struct Light {
    Vector3 pos;
    Color color;
} Light;

// typedef enum Object_Type {
//     OBJ_SPHERE, OBJ_PLANE
// } Object_Type;
// 
// struct Object {
//     Object_Type type;
//     union {
//         Sphere sphere;
//         Plane plane;
//     }
// }

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

Vector3 sphere_normal (Sphere sphere, Vector3 point) {
    return vec3_normalize(vec3_sub(point, sphere.pos));
}

Vector3 parametric_line (float t, Ray ray) {
    Vector3 offset = vec3_mul(ray.dir, t);
    Vector3 point = vec3_add(ray.start, offset);
    return point;
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

Sphere spheres[3];
Light lights[2];

void setup_scene () {
    spheres[0].pos.x = 4.0f;
    spheres[0].pos.z = 25.0f;
    spheres[0].pos.y = 1.0f;
    spheres[0].r = 3.0f;

    spheres[1].pos.x = -4.0f;
    spheres[1].pos.z = 25.0f;
    spheres[1].pos.y = 1.0f;
    spheres[1].r = 3.0f;

    spheres[2].pos.x = 0.0f;
    spheres[2].pos.z = 25.0f;
    spheres[2].pos.y = 3.0f;
    spheres[2].r = 3.0f;

    lights[0].color.r = 0.5f;
    lights[0].color.g = 1.0f;
    lights[0].color.b = 1.0f;

    lights[0].pos.x = 20;
    lights[0].pos.y = -20;
    lights[0].pos.z = 25;

    lights[1].color.r = 1.0f;
    lights[1].color.g = 0.5f;
    lights[1].color.b = 0.0f;

    lights[1].pos.x = 0;
    lights[1].pos.y = 0;
    lights[1].pos.z = 5;
}

bool intersect_scene (Ray ray, float *hit, int *hit_object) {
    float closest_hit = INFINITY;
    int closest_hit_object = -1;
    bool found_anything = false;

    for (int i = 0; i < ARRAY_LEN(spheres); i++) {
        float this_hit;
        
        if (intersect_sphere(ray, spheres[i], &this_hit)) {
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

    for (int i = 0; i < ARRAY_LEN(spheres); i++) {
        if (i == exception) continue;

        float this_hit;
        
        if (intersect_sphere(ray, spheres[i], &this_hit)) {
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

Color color_from_light (Light light, Sphere sphere, Vector3 point) {
    Color result = {0};
    Vector3 normal = sphere_normal(sphere, point);

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

Color color_from_all_lights (int object_index, Vector3 point) {
    Color result = {0};

    Sphere sphere = spheres[object_index];

    for (int i = 0; i < ARRAY_LEN(lights); i++) {
        Color this_color = color_from_light(lights[i], sphere, point);

        Ray shadow_ray = {0};
        shadow_ray.start = point;
        shadow_ray.dir = vec3_sub(lights[i].pos, point);

        bool did_we_hit = intersect_scene_with_one_exception(
            shadow_ray, NULL, NULL, object_index);
        
        if (!did_we_hit)
            result = color_add(result, this_color);
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

            float hit;
            int hit_object;
            
            if (intersect_scene(sight, &hit, &hit_object)) {
                Vector3 hit_point = parametric_line(hit, sight);

                Color surface_color = color_from_all_lights(
                    hit_object, hit_point
                ); 

                u8 green = (u8)(surface_color.g * 255);
                u8 blue  = (u8)(surface_color.b * 255);
                u8 red   = (u8)(surface_color.r * 255);

                *pixel++ = ((red << 16) | (green << 8) | blue);
            } else {
                *pixel++ =  0; // background
            }
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

    int window_width = 1000;
    int window_height = 700;

    win32_resize_dib_section(&global_backbuffer, window_width, window_height);

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
    
    raytrace(&global_backbuffer);

    global_running = true;
    while(global_running) {

        MSG message;

        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
                global_running = false;

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