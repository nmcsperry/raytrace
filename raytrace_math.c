#include <stdio.h>
#include <math.h>
#include <windows.h>

#define DEBUG_PRINT(...) { \
    char _debug_str[256]; \
    sprintf(_debug_str, __VA_ARGS__); \
    OutputDebugString(_debug_str); \
}

#define ARRAY_LEN(x) sizeof((x))/sizeof((x)[0])

#define EPSILON 0.0004

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
    float metalness;
    float specularness;
    float diffuseness;
    float shinyness;
    int refract;
    float refract_amount;
} Material;

typedef struct Sphere {
    float r;
    Vector3 pos;
} Sphere;

typedef struct Plane {
    Vector3 pos;
    Vector3 normal;
} Plane;

typedef struct Checkerboard {
    Plane plane;
    Material material_2;
    float scale;
} Checkerboard;

typedef struct Compound_Sphere {
    Sphere real_sphere;
    Sphere anti_sphere;
} Compound_Sphere;

typedef struct Ray {
    Vector3 pos;
    Vector3 dir;
} Ray;

typedef struct Light {
    Vector3 pos;
    Color color;
} Light;

typedef enum Object_Type {
    OBJ_SPHERE, OBJ_PLANE, OBJ_CHECKERBOARD, OBJ_COMPOUNDSPHERE
} Object_Type;

typedef struct Object {
    Object_Type type;
    union {
        Sphere sphere;
        Plane plane;
        Checkerboard checkerboard;
        Compound_Sphere compound_sphere;
    };
    Material material;
} Object;

// global scene variables

Object scene[6];
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
    float epsilon = EPSILON;
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

Color color_scale (Color a, float b) {
    return (Color) {a.r * b, a.g * b, a.b * b};
}

Color color_lerp (Color a, Color b, float alpha) {
    return (Color) {
        (1.0f - alpha) * a.r + alpha * b.r,
        (1.0f - alpha) * a.g + alpha * b.g,
        (1.0f - alpha) * a.b + alpha * b.b,
    };
}

// shapes

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

bool intersect_anti_sphere (Ray ray, Sphere sphere, float *parametric_hit) {
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
                if (answers[0] > answers[1])
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

bool inside_plane(Vector3 point, Plane plane) {
    float plane_offset = vec3_dot(plane.normal, plane.pos);

    float dist = point.x * plane.pos.x + point.y * plane.pos.y + point.z * plane.pos.z;
    if (dist < plane_offset)
        return true;
    return false;
}

bool inside_sphere(Vector3 point, Sphere sphere) {
    float dist = sq(point.x - sphere.pos.x) + sq(point.y - sphere.pos.y) + sq(point.z - sphere.pos.z);
    if (dist < sq(sphere.r))
        return true;
    return false;
}

bool inside_object (Vector3 point, Object object) {
    switch(object.type) {
        case OBJ_SPHERE:
            return inside_sphere(point, object.sphere);
        case OBJ_PLANE:
            return inside_plane(point, object.plane);
        case OBJ_CHECKERBOARD:
            return inside_plane(point, object.checkerboard.plane);
        default:
            return false;
    }
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

Material checkerboard_choose_material (Object object, Vector3 point) {
    Checkerboard checkerboard = object.checkerboard;

    Vector3 v0 = (Vector3) {1.0f, 0.0f, 0.0f};   
    Vector3 u = vec3_normalize(vec3_cross(checkerboard.plane.normal, v0));
    Vector3 v = vec3_normalize(vec3_cross(checkerboard.plane.normal, u));

    Vector3 ref = vec3_sub(point, checkerboard.plane.pos);

    int ui = (int) ceil(vec3_dot(u, ref) / checkerboard.scale);
    int vi = (int) ceil(vec3_dot(v, ref) / checkerboard.scale);

    if ((unsigned)ui % 2 != (unsigned)vi % 2)
        return object.material;
    else
        return checkerboard.material_2;
}

Material object_material (Object object, Vector3 point) {
    Material material;
    switch (object.type) {
        case OBJ_COMPOUNDSPHERE:
        case OBJ_SPHERE:
        case OBJ_PLANE:
            material = object.material;
            break;
        case OBJ_CHECKERBOARD:
            material = checkerboard_choose_material(
                object, point);
            break;
    }

    return material;
}

bool intersect_object (Ray ray, Object object, float *hit, Vector3 *hit_normal) {
    bool intersect = false;
    switch (object.type) {
        case OBJ_SPHERE: {
            intersect = intersect_sphere(ray, object.sphere, hit);
            Vector3 hit_point = parametric_line(*hit, ray);
            *hit_normal = sphere_normal(object.sphere, hit_point);
        } break;
        case OBJ_COMPOUNDSPHERE: {
            Sphere real_sphere = object.compound_sphere.real_sphere;
            Sphere anti_sphere = object.compound_sphere.anti_sphere;

            float initial_hit;
            intersect = intersect_sphere(ray, real_sphere, &initial_hit);
            if (!intersect) return false;

            Vector3 hit_point = parametric_line(initial_hit, ray);

            if (inside_sphere(hit_point, anti_sphere)) {
                intersect = false;

                Ray new_ray;
                new_ray.pos = ray.pos;//vec3_add(ray.pos, vec3_mul(ray.dir, 5));
                new_ray.dir = ray.dir;

                float anti_hit;
                if (intersect_anti_sphere(new_ray, anti_sphere, &anti_hit)) {
                    Vector3 anti_hit_point = parametric_line(anti_hit, new_ray);
                    if (inside_sphere(anti_hit_point, real_sphere)) {
                        *hit = anti_hit;
                        *hit_normal = vec3_mul(sphere_normal(anti_sphere, anti_hit_point), -1.0f);
                        intersect = true;
                    }
                }
            } else {
                *hit = initial_hit;
                *hit_normal = sphere_normal(real_sphere, hit_point);
            }
        } break;
        case OBJ_PLANE: {
            intersect = intersect_plane(ray, object.plane, hit);
            Vector3 hit_point = parametric_line(*hit, ray);
            *hit_normal = object_normal(object, hit_point);
        } break;
        case OBJ_CHECKERBOARD: {
            intersect = intersect_plane(ray, object.checkerboard.plane, hit);
            Vector3 hit_point = parametric_line(*hit, ray);
            *hit_normal = object_normal(object, hit_point);
        } break;
    }

    
    return intersect;
}

bool intersect_scene_with_except (
    Ray ray, float *hit, int *hit_object, Vector3 *hit_normal, int exception
) {
    float closest_hit = INFINITY;
    int closest_hit_object = -1;
    Vector3 closest_hit_normal;

    bool found_anything = false;

    for (int i = 0; i < ARRAY_LEN(scene); i++) {
        if (i == exception) continue;

        float this_hit;
        Vector3 this_hit_normal;

        bool intersect = intersect_object(ray, scene[i], &this_hit, &this_hit_normal);
        
        if (intersect) {
            found_anything = true;

            if (this_hit < closest_hit) {
                closest_hit = this_hit;
                closest_hit_object = i;
                closest_hit_normal = this_hit_normal;
            }
        }
    }

    if (found_anything) {
        if (hit) *hit = closest_hit;
        if (hit_object) *hit_object = closest_hit_object;
        if (hit_normal) *hit_normal = closest_hit_normal;
    }

    return found_anything;
}

bool intersect_scene (Ray ray, float *hit, int *hit_object, Vector3 *hit_normal) {
    return intersect_scene_with_except (ray, hit, hit_object, hit_normal, -1);
}

Color diffuse_from_light (Light light, Object object, Vector3 point, Vector3 normal) {
    Color result = {0};

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

Color specular_from_light (
    Light light, Object object, Vector3 point, Vector3 normal, Ray sight, Material material
) {
    Color result = {0};
    
    Vector3 sight_dir = vec3_normalize(sight.dir);
    Vector3 light_dir = vec3_normalize(vec3_sub(point, light.pos));

    // dir - normal * 2 (dir . normal)
    Vector3 reflect_light_dir = vec3_sub(light_dir, vec3_mul(normal, 2.0f * vec3_dot(light_dir, normal)));
    reflect_light_dir = vec3_normalize(reflect_light_dir);

    float lightness = -1.0f * vec3_dot(reflect_light_dir, sight_dir);

    float sm = material.metalness;
    Color specular_color = color_add(color_scale(material.color, sm), color_scale((Color) {1, 1, 1}, 1 - sm));

    if (lightness > 0) {
        result.g = pow(lightness, material.shinyness) * light.color.g;
        result.b = pow(lightness, material.shinyness) * light.color.b;
        result.r = pow(lightness, material.shinyness) * light.color.r;
    }

    return result;
}

Color color_from_all_lights (int object_index, Vector3 point, Vector3 normal, Ray sight, Color object_color) {
    Color result = {0};

    Object object = scene[object_index];

    Material material = object_material(object, point);

    for (int i = 0; i < ARRAY_LEN(lights); i++) {
        Vector3 point_to_light = vec3_sub(lights[i].pos, point);

        Ray shadow_ray = {0};
        shadow_ray.dir = vec3_normalize(point_to_light);
        shadow_ray.pos = vec3_add(point, vec3_mul(shadow_ray.dir, EPSILON));

        float shadow_hit;
        int hit_object;
        bool did_we_hit = intersect_scene(shadow_ray, &shadow_hit, &hit_object, NULL);
        if (did_we_hit) {
            float light_hit = vec3_dot(point_to_light, point_to_light);
            if (light_hit < sq(shadow_hit)) did_we_hit = false;
        }
        if (did_we_hit) {
            if (scene[hit_object].material.refract) did_we_hit = false;
        }
        
        if (!did_we_hit) {
            Color diffuse_comp = diffuse_from_light(lights[i], object, point, normal);
            Color diffuse = color_scale(diffuse_comp, material.diffuseness);

            Color specular_comp = specular_from_light(lights[i], object, point, normal, sight, material);
            Color specular = color_scale(specular_comp, material.specularness);

            result = color_add(result, color_mul(diffuse, object_color));
            result = color_add(result, specular);
        }
    }
    
    return result; 
}

Color ray_color_with_except (Ray sight, int depth, int exception) {
    Color result = {0};

    if (depth > 20) {
        result = (Color) {1.0f, 1.0f, 1.0f};
        return result;
    }

    float hit;
    int hit_object;

    Vector3 normal;
    
    if (intersect_scene_with_except(sight, &hit, &hit_object, &normal, exception)) {
        Vector3 hit_point = parametric_line(hit, sight);

        Object object = scene[hit_object];
        // normal = object_normal(object, hit_point);

        float mirror = object_material(object, hit_point).mirror;
        int refract = object_material(object, hit_point).refract;
        float refract_amount = object_material(object, hit_point).refract_amount;

        if (refract) {
            if (inside_object(
                vec3_sub(hit_point, vec3_mul(sight.dir, EPSILON)),
                object
            )) {
                refract_amount = 1 / refract_amount;
                normal = vec3_mul(normal, -1);
            }

            Vector3 dir = vec3_normalize(sight.dir);

            float c1 = -vec3_dot(dir, normal);
            float c2 = sqrt(1 - sq(refract_amount) * (1 - sq(c1)));
            Vector3 refraction_dir = vec3_add(
                vec3_mul(dir, refract_amount),
                vec3_mul(normal, refract_amount * c1 - c2)
            );

            Ray refraction = {0};
            refraction.dir = vec3_normalize(refraction_dir);
            refraction.pos = vec3_add(hit_point, vec3_mul(refraction.dir, EPSILON));

            Color refraction_color = ray_color_with_except(refraction, depth + 1, -1);
            // Color light_color = color_from_all_lights(
            //     hit_object, hit_point, normal, sight, refraction_color); 

            result = refraction_color;
        }
        else if (mirror > 0.0f) {
            Vector3 dir = vec3_normalize(sight.dir);
        
            // dir - normal * 2 (dir . normal)

            Vector3 reflection_dir = vec3_sub(dir, vec3_mul(normal, 2.0f * vec3_dot(dir, normal)));
            Ray reflection = {0};
            reflection.dir = vec3_normalize(reflection_dir);
            reflection.pos = vec3_add(hit_point, vec3_mul(reflection.dir, EPSILON));

            Color mirror_color = ray_color_with_except(reflection, depth + 1, -1);
            Color base_color = object_material(object, hit_point).color;
            Color object_color = color_lerp(base_color, mirror_color, mirror);

            Color light_color = color_from_all_lights(
                hit_object, hit_point, normal, sight, object_color); 

            result = light_color;
        } else {
            Color base_color = object_material(object, hit_point).color;
            result = color_from_all_lights(hit_object, hit_point, normal, sight, base_color); 
        }
    }

    return result;
}

