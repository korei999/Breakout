#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN 1
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <windows.h>
#undef near
#undef far

#include "IWindow.hh"


namespace platform
{
namespace win32
{

struct Win32Window
{
    IWindow super;
    HINSTANCE hInstance;
    HWND hWindow;
    HDC hDeviceContext;
    HGLRC hGlContext;
    WNDCLASSEXW windowClass;
    RAWINPUTDEVICE rawInputDevices[2];

    Win32Window() = default;
    Win32Window(String name, HINSTANCE instance);
};

void Win32Start(Win32Window* s);
void Win32Destroy(Win32Window* s);
void Win32EnableRelativeMode(Win32Window* s);
void Win32DisableRelativeMode(Win32Window* s);
void Win32SetCursorImage(Win32Window* s, String cursorType);
void Win32SetFullscreen(Win32Window* s);
void Win32UnsetFullscreen(Win32Window* s);
void Win32TogglePointerRelativeMode(Win32Window* s);
void Win32ToggleFullscreen(Win32Window* s);
void Win32HideCursor(Win32Window* s);
void Win32BindGlContext(Win32Window* s);
void Win32UnbindGlContext(Win32Window* s);
void Win32SetSwapInterval(Win32Window* s, int interval);
void Win32ToggleVSync(Win32Window* s);
void Win32SwapBuffers(Win32Window* s);
void Win32ProcEvents(Win32Window* s);
void Win32ShowWindow(Win32Window* s);

} /* namespace win32 */
} /* namespace platform */

inline void WindowStart(platform::win32::Win32Window* s) { Win32Start(s); }
inline void WindowDisableRelativeMode(platform::win32::Win32Window* s) { Win32DisableRelativeMode(s); }
inline void WindowEnableRelativeMode(platform::win32::Win32Window* s) { Win32EnableRelativeMode(s); }
inline void WindowTogglePointerRelativeMode(platform::win32::Win32Window* s) { Win32TogglePointerRelativeMode(s); }
inline void WindowToggleFullscreen(platform::win32::Win32Window* s) { Win32ToggleFullscreen(s); }
inline void WindowHideCursor(platform::win32::Win32Window* s) { Win32HideCursor(s); }
inline void WindowSetCursorImage(platform::win32::Win32Window* s, String cursorType) { Win32SetCursorImage(s, cursorType); }
inline void WindowSetFullscreen(platform::win32::Win32Window* s) { Win32SetFullscreen(s); }
inline void WindowUnsetFullscreen(platform::win32::Win32Window* s) { Win32UnsetFullscreen(s); }
inline void WindowBindGlContext(platform::win32::Win32Window* s) { Win32BindGlContext(s); }
inline void WindowUnbindGlContext(platform::win32::Win32Window* s) { Win32UnbindGlContext(s); }
inline void WindowSetSwapInterval(platform::win32::Win32Window* c, int interval) { Win32SetSwapInterval(c, interval); }
inline void WindowToggleVSync(platform::win32::Win32Window* s) { Win32ToggleVSync(s); }
inline void WindowSwapBuffers(platform::win32::Win32Window* s) { Win32SwapBuffers(s); }
inline void WindowProcEvents(platform::win32::Win32Window* s) { Win32ProcEvents(s); }
inline void WindowShowWindow(platform::win32::Win32Window* s) { Win32ShowWindow(s); }
inline void WindowDestroy(platform::win32::Win32Window* s) { Win32Destroy(s); }
