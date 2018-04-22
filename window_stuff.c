#include <windows.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

typedef s32 bool;
#define true (bool)1;
#define false (bool)0;

typedef struct Win32_Offscreen_Buffer {
    BITMAPINFO info;
    void * memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
} Win32_Offscreen_Buffer;

typedef struct Win32_Window_Dimension {
    int width;
    int height;
} Win32_Window_Dimension;

Win32_Offscreen_Buffer global_backbuffer;
bool global_running;

inline Win32_Window_Dimension win32_get_window_dimension (HWND window) {
    Win32_Window_Dimension result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;

    result.width = width;
    result.height = height;

    return result;
}

void render_weird_gradient (Win32_Offscreen_Buffer * buffer, int xoff, int yoff) {
    u8 * row = (u8 *) buffer->memory;

    for (int y = 0; y < buffer->height; ++y) {
    
        u32 * pixel = (u32 *) row; 
        for (int x = 0; x < buffer->width; ++x) {
            u8 green = x + xoff;
            u8 blue = y + yoff;
            u8 red = x + xoff;

            *pixel++ = ((red << 16) | (blue << 8) | green);
        }
        row += buffer->pitch;
    }
}

void win32_resize_dib_section(Win32_Offscreen_Buffer * buffer, int width, int height) {
    
    if (buffer->memory) {
        // VirtualProtect is another thing for (debugging?)
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    buffer->bytes_per_pixel = 4;
    buffer->pitch = width * buffer->bytes_per_pixel;

    // virtual alloc: allocates certain number of pages, pages are big blocks of memory
    // mem_commit: means we are actually going to use it and aren't just allocating it for later 

    int bitmap_memory_size = width * height * buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void win32_display_buffer_in_window (
    HDC device_context,
    int window_width,
    int window_height,
    Win32_Offscreen_Buffer * buffer,
    int x, int y,
    int width, int height
) {
    /*
    x, y, width, height,
    x, y, width, height,
    */

    // i think this renders to the window
    StretchDIBits(
        device_context,
        0, 0, window_width, window_height,
        0, 0, buffer->width, buffer->height,
        buffer->memory,
        &buffer->info,
        DIB_RGB_COLORS, SRCCOPY
    );
    
}

LRESULT CALLBACK
win32_main_window_callback(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    LRESULT result = 0;

    switch (message) {

        case WM_DESTROY: {
            global_running = false;
        } break;

        case WM_CLOSE: {
            global_running = false;
        } break;

        case WM_ACTIVATEAPP: {
            //OutputDebugStringA("WM_ACTIVATE\n");
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            RECT client_rect;
            GetClientRect(window, &client_rect);

            Win32_Window_Dimension dimension = win32_get_window_dimension(window);
            win32_display_buffer_in_window(device_context, dimension.width, dimension.height, &global_backbuffer, x, y, width, height);

            EndPaint(window, &paint);
            //OutputDebugStringA("WM_PAINT\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            u32 vk_code = w_param;
            bool was_down = (l_param & (1 << 30)) != 0;
            bool is_down = (l_param & (1 << 31)) == 0;

            if (is_down != was_down) 
            {
                if (vk_code == 'W')
                {
                }
                else if (vk_code == 'A')
                {
                }
                else if (vk_code == 'S')
                {
                }
                else if (vk_code == 'D')
                {
                }
                else if (vk_code == 'Q')
                {
                }
                else if (vk_code == 'E')
                {
                }
                else if (vk_code == VK_UP)
                {
                }
                else if (vk_code == VK_LEFT)
                {
                }
                else if (vk_code == VK_DOWN)
                {
                }
                else if (vk_code == VK_RIGHT)
                {
                }
                else if (vk_code == VK_ESCAPE)
                {
                    OutputDebugStringA("escape: ");

                    if (is_down)
                        OutputDebugStringA("is\n");
                    if (was_down)
                        OutputDebugStringA("was\n");
                }
                else if (vk_code == VK_SPACE)
                {
                }
            }

            bool alt_down = (l_param & (1 << 29)) != 0;
            if (vk_code == VK_F4 && alt_down)
            {
                global_running = false;
            }
        } break;

        default: {
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }

    return result;
}
