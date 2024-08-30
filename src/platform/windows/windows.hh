#pragma once

#include <windows.h>

#ifdef min
    #undef min
#endif
#ifdef max
    #undef max
#endif
#ifdef near
    #undef near
#endif
#ifdef far
    #undef far
#endif

#include "App.hh"

namespace platform
{
namespace win32
{

struct Window
{
    App base;
    HINSTANCE _hInstance;
    HWND _hWindow;
    HDC _hDeviceContext;
    HGLRC _hGlContext;
    WNDCLASSEXW _windowClass;
    RAWINPUTDEVICE _rawInputDevices[2];

    Window() = default;
    Window(String name, HINSTANCE instance);
};

void WindowDestroy(Window* s);
void WindowInit(Window* s);
void WindowEnableRelativeMode(Window* s);
void WindowDisableRelativeMode(Window* s);
void WindowSetCursorImage(Window* s, String cursorType);
void WindowSetFullscreen(Window* s);
void WindowUnsetFullscreen(Window* s);
void WindowTogglePointerRelativeMode(Window* s);
void WindowToggleFullscreen(Window* s);
void WindowBindGlContext(Window* s);
void WindowUnbindGlContext(Window* s);
void WindowSetSwapInterval(Window* s, int interval);
void WindowToggleVSync(Window* s);
void WindowSwapBuffers(Window* s);
void WindowProcEvents([[maybe_unused]] Window* s);
void WindowShowWindow([[maybe_unused]] Window* s);

} /* namespace win32 */
} /* namespace platform */
