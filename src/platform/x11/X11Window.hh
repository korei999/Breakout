#pragma once

#include "IWindow.hh"

#include <X11/Xlib.h>
#include <EGL/egl.h>

namespace platform
{
namespace x11
{

struct Window
{
    IWindow super {};
    Display* pDisplay {};
    ::Window window {};
    EGLDisplay pEGLDisplay {};
    EGLSurface pEGLSurface {};
    EGLContext pEGLContext {};

    Window() = delete;
    Window(String sName);
};

void WindowStart(Window* s);
void WindowDestroy(Window* s);
void WindowEnableRelativeMode(Window* s);
void WindowDisableRelativeMode(Window* s);
void WindowHideCursor(Window* s);
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
void WindowProcEvents(Window* s);
void WindowShowWindow(Window* s);

} /* namespace x11 */
} /* namespace platform */

inline void WindowStart(platform::x11::Window* s) { platform::x11::WindowStart(s); }
inline void WindowDisableRelativeMode(platform::x11::Window* s) { platform::x11::WindowDisableRelativeMode(s); }
inline void WindowEnableRelativeMode(platform::x11::Window* s) { platform::x11::WindowEnableRelativeMode(s); }
inline void WindowTogglePointerRelativeMode(platform::x11::Window* s) { platform::x11::WindowTogglePointerRelativeMode(s); }
inline void WindowToggleFullscreen(platform::x11::Window* s) { platform::x11::WindowToggleFullscreen(s); }
inline void WindowHideCursor(platform::x11::Window* s) { platform::x11::WindowHideCursor(s); }
inline void WindowSetCursorImage(platform::x11::Window* s, String cursorType) { platform::x11::WindowSetCursorImage(s, cursorType); }
inline void WindowSetFullscreen(platform::x11::Window* s) { platform::x11::WindowSetFullscreen(s); }
inline void WindowUnsetFullscreen(platform::x11::Window* s) { platform::x11::WindowUnsetFullscreen(s); }
inline void WindowBindGlContext(platform::x11::Window* s) { platform::x11::WindowBindGlContext(s); }
inline void WindowUnbindGlContext(platform::x11::Window* s) { platform::x11::WindowUnbindGlContext(s); }
inline void WindowSetSwapInterval(platform::x11::Window* c, int interval) { platform::x11::WindowSetSwapInterval(c, interval); }
inline void WindowToggleVSync(platform::x11::Window* s) { platform::x11::WindowToggleVSync(s); }
inline void WindowSwapBuffers(platform::x11::Window* s) { platform::x11::WindowSwapBuffers(s); }
inline void WindowProcEvents(platform::x11::Window* s) { platform::x11::WindowProcEvents(s); }
inline void WindowShowWindow(platform::x11::Window* s) { platform::x11::WindowShowWindow(s); }
inline void WindowDestroy(platform::x11::Window* s) { platform::x11::WindowDestroy(s); }
