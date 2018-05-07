#include "window_stuff.c"
#include "raytrace_math.c"

// setup scene

#define MAT_DEFAULT(obj) obj.color = (Color) {1.0f, 1.0f, 1.0f}, obj.mirror = 0.0f, \
obj.diffuseness = 1.0f, obj.specularness = 0.4f, obj.shinyness = 4.0f, obj.metalness = 0.2f

void setup_scene () {
    scene[1] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {8.0f, 1.5f, 22.5f},
        .sphere.r = 3.0f,

        MAT_DEFAULT(.material),
        .material.color = (Color) {1.0f, 0.3f, 0.3f},
    };

    scene[2] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {0.0f, 3.0f, 25.0f},
        .sphere.r = 6.0f,

        MAT_DEFAULT(.material),
        .material.color = (Color) {0.5f, 0.5f, 1.0f},
        .material.mirror = 0.8f,
        .material.specularness = 1.0f,
        .material.diffuseness = 1.0f,
        .material.shinyness = 30.0f,
        .material.metalness = 1.0f
    };

    scene[0] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {-9.0f, 1.2f, 25.0f},
        .sphere.r = 4.0f,

        MAT_DEFAULT(.material),
        .material.specularness = 0.1,
        .material.diffuseness  = 0.8,
        .material.color = (Color) {0.3f, 1.0f, 0.3f},
    };

    scene[4] = (Object) {
        .type = OBJ_COMPOUNDSPHERE,

        .compound_sphere.real_sphere.pos = (Vector3) {-2.0f, -7.0f, 19.0f},
        .compound_sphere.real_sphere.r = 4.0f,
        .compound_sphere.anti_sphere.pos = (Vector3) {-2.0f, -0.5f, 17.0f},
        .compound_sphere.anti_sphere.r = 5.0f,

        MAT_DEFAULT(.material),
        .material.color = (Color) {0.8f, 0.3f, 0.8f},
        .material.mirror = 0.8f
    };

    scene[5] = (Object) {
        .type = OBJ_SPHERE,

        .sphere.pos = (Vector3) {-2.0f, -4.0f, 16.0f},
        .sphere.r = 0.5f,

        MAT_DEFAULT(.material),
        .material.specularness = 0.1,
        .material.diffuseness  = 0.8,
        .material.color = (Color) {0.3f, 1.0f, 0.3f},
    };

    scene[3] = (Object) {
        .type = OBJ_CHECKERBOARD,

        .checkerboard.plane.pos = (Vector3) {0.0f, 3.0f, 27.0f},
        .checkerboard.plane.normal = (Vector3) {-0.5f, 1.0f, -1.0f},

        MAT_DEFAULT(.material),
        .material.color = (Color) {1.0f, 1.0f, 1.0f},

        MAT_DEFAULT(.checkerboard.material_2),
        .checkerboard.material_2.color = (Color) {0.3f, 0.3f, 0.3f},

        .checkerboard.scale = 5.0f
    };
    scene[3].plane.normal = vec3_normalize(scene[3].plane.normal);

    lights[0] = (Light) {
        .color = (Color) {0.5f, 1.0f, 1.0f},
        .pos = (Vector3) {20.0f, 15.0f, 15.0f}
    };

    lights[1] = (Light) {
        .color = (Color) {0.7f, 0.7f, 0.5f},
        .pos = (Vector3) {5.0f, 0.0f, 5.0f}
    };
}

void raytrace (Win32_Offscreen_Buffer *buffer) {
    u8 * row = (u8 *) buffer->memory;

    Ray camera = (Ray) {
        .pos = (Vector3) {0.0f, 0.0f, 1.0f},
        .dir = (Vector3) {0.0f, 0.0f, 1.0f}
    };

    for (int y = 0; y < buffer->height; ++y) {
    
        u32 * pixel = (u32 *) row; 
        for (int x = 0; x < buffer->width; ++x) {

            Ray sight = camera;
            sight.dir.x += (-(float)x + buffer->width/2 ) / buffer->height * 1.2f;
            sight.dir.y += (-(float)y + buffer->height/2) / buffer->height * 1.2f;
            
            Color surface_color = ray_color_with_except(sight, 0, -1);

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
