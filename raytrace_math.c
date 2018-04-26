#include <stdio.h>
#include <math.h>

#define DEBUG_PRINT(...) { \
    char _debug_str[256]; \
    sprintf(_debug_str, __VA_ARGS__); \
    OutputDebugString(_debug_str); \
}

#define ARRAY_LEN(x) sizeof((x))/sizeof((x)[0])

// types

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

typedef struct Torus {
    float inner_r;
    float outer_r;
    Vector3 pos;
    Material material;
} Torus;

typedef struct Ray {
    Vector3 pos;
    Vector3 dir;
} Ray;

typedef struct Light {
    Vector3 pos;
    Color color;
} Light;

typedef enum Object_Type {
    OBJ_SPHERE, OBJ_PLANE, OBJ_CHECKERBOARD, OBJ_TORUS
} Object_Type;

typedef struct Object {
    Object_Type type;
    union {
        Sphere sphere;
        Plane plane;
        Checkerboard checkerboard;
        Torus torus;
    };
} Object;

// global scene variables

Object scene[4];
Light lights[3];

// float math

float sq (float a) {
    return a*a;
}

float fclamp (float a, float max, float min) {
    if (a > max) return max;
    if (a < min) return min;
    return a;
}

bool fequal (float a, float b) {
    float epsilon = 0.0005;
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

// vectors

Vector3 vec3_add (Vector3 a, Vector3 b) {
    return (Vector3) {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 vec3_sub (Vector3 a, Vector3 b) {
    return (Vector3) {a.x - b.x, a.y - b.y, a.z - b.z};
}

float vec3_dot (Vector3 a, Vector3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vector3 vec3_div (Vector3 a, float b) {
    return (Vector3) {a.x / b, a.y / b, a.z / b};
}

Vector3 vec3_mul (Vector3 a, float b) {
    return (Vector3) {a.x * b, a.y * b, a.z * b};
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

// shapes

Vector3 sphere_normal (Sphere sphere, Vector3 point) {
    return vec3_normalize(vec3_sub(point, sphere.pos));
}

Vector3 torus_normal (Torus torus, Vector3 point) {
    for (int i = 0; i < 360; i++) {
        float rad = (float)i / 180 * 3.14159f;
        float middle_r = (torus.inner_r + torus.outer_r) / 2;
        Vector3 offset = {cos(rad) * middle_r, sin(rad) * middle_r, 0.0f};

        Sphere new_sphere = (Sphere) {
            .pos = vec3_add(torus.pos, offset),
            .r = (torus.outer_r - torus.inner_r) / 2,
        };
        
        Vector3 diff = vec3_sub(new_sphere.pos, point);
        if (fequal(vec3_dot(diff, diff), new_sphere.r * new_sphere.r)) {
            return sphere_normal(new_sphere, point);
        }
    }

    return (Vector3) {0, 0, 0};
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
        case OBJ_TORUS:
            normal = torus_normal(object.torus, point);
            break;
    }

    return normal;
}

Vector3 parametric_line (float t, Ray ray) {
    Vector3 offset = vec3_mul(ray.dir, t);
    Vector3 point = vec3_add(ray.pos, offset);
    return point;
}

bool intersect_plane (Ray ray, Plane plane, float *parametric_hit) {
    float plane_offset = vec3_dot(plane.normal, plane.pos);

    float x = (plane_offset - vec3_dot(plane.normal, ray.pos))
        / vec3_dot(plane.normal, ray.dir);

    if (x > 0) {
        if (parametric_hit != NULL) *parametric_hit = x;
        return true;
    }

    return false;
}

bool intersect_sphere (Ray ray, Sphere sphere, float *parametric_hit) {
    Vector3 sphere_off = vec3_sub(ray.pos, sphere.pos);

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

bool intersect_torus (
    Ray ray, Torus torus, float *parametric_hit, int *sphere_hit
) {
    Sphere test_sphere = (Sphere) {
        .pos = torus.pos,
        .r = torus.outer_r,
        .material = torus.material
    }; 

    if (!intersect_sphere(ray, test_sphere, NULL))
        return false;

    float first_hit = INFINITY;
    int hit_index = -1;

    for (int i = 0; i < 360; i++) {
        float this_hit;
        float rad = (float)i / 180 * 3.14159f;
        float middle_r = (torus.inner_r + torus.outer_r) / 2;
        Vector3 offset = {cos(rad) * middle_r, sin(rad) * middle_r, 0.0f};

        Sphere new_sphere = (Sphere) {
            .pos = vec3_add(torus.pos, offset),
            .r = (torus.outer_r - torus.inner_r) / 2,
            .material = torus.material
        };

        if (intersect_sphere(ray, new_sphere, &this_hit))
            if (this_hit < first_hit) {
                first_hit = this_hit;
                hit_index = i;
            }
    }

    if (first_hit != INFINITY) {
        if (parametric_hit != NULL) *parametric_hit = first_hit;
        if (sphere_hit != NULL) *sphere_hit = hit_index;
        return true;
    }

    return false;
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
        case OBJ_TORUS:
            material = object.torus.material;
            break;
    }

    return material;
}

// colors and matherials

Color color_add (Color a, Color b) {
    return (Color) {
        fclamp(a.r + b.r, 1.0f, 0.0f),
        fclamp(a.g + b.g, 1.0f, 0.0f),
        fclamp(a.b + b.b, 1.0f, 0.0f)
    };
}

Color color_mul (Color a, Color b) {
    return (Color) {a.r * b.r, a.g * b.g, a.b * b.b};
}

Color color_lerp (Color a, Color b, float alpha) {
    return (Color) {
        (1.0f - alpha) * a.r + alpha * b.r,
        (1.0f - alpha) * a.g + alpha * b.g,
        (1.0f - alpha) * a.b + alpha * b.b,
    };
}


bool intersect_object (Ray ray, Object object, float *hit) {
    bool intersect = false;
    switch (object.type) {
        case OBJ_SPHERE:
            intersect = intersect_sphere(ray, object.sphere, hit);
            break;
        case OBJ_PLANE:
            intersect = intersect_plane(ray, object.plane, hit);
            break;
        case OBJ_CHECKERBOARD:
            intersect = intersect_plane(ray, object.checkerboard.plane, hit);
            break;
        case OBJ_TORUS:
            intersect = intersect_torus(ray, object.torus, hit, NULL);
            break;
    }
    
    return intersect;
}

bool intersect_scene (Ray ray, float *hit, int *hit_object) {
    float closest_hit = INFINITY;
    int closest_hit_object = -1;
    bool found_anything = false;

    for (int i = 0; i < ARRAY_LEN(scene); i++) {
        float this_hit;

        bool intersect = intersect_object(ray, scene[i], &this_hit);
        
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

        bool intersect = intersect_object(ray, scene[i], &this_hit);
        
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

Color color_from_all_lights (int object_index, Vector3 point) {
    Color result = {0};

    Object object = scene[object_index];

    for (int i = 0; i < ARRAY_LEN(lights); i++) {
        Color this_color = color_from_light(lights[i], object, point);

        Ray shadow_ray = {0};
        shadow_ray.pos = point;
        shadow_ray.dir = vec3_sub(lights[i].pos, point);

        bool did_we_hit = intersect_scene_with_one_exception(
            shadow_ray, NULL, NULL, object_index);
        
        if (!did_we_hit)
            result = color_add(result, this_color);
    }
    
    return color_mul(result, object_material(object, point).color);
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
            reflection.pos = hit_point;

            Color mirror_color =
                get_ray_color_with_one_exception(reflection, depth + 1, hit_object);
            Color light_color = color_from_all_lights(hit_object, hit_point); 

            result = color_lerp(light_color, mirror_color, mirror);
            // result = color_add(light_color, mirror_color);
        } else {
            result = color_from_all_lights(hit_object, hit_point); 
        }
    }

    return result;
}
