#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl-core.h>

#include "wayland-protocols/pointer-constraints-unstable-v1.h"
#include "wayland-protocols/relative-pointer-unstable-v1.h"
#include "wayland-protocols/xdg-shell.h"

#include "Window.hh"

#include "adt/types.hh"

using namespace adt;

namespace platform
{
namespace wayland
{

struct Client
{
    Window super;
    wl_display* display {};
    wl_registry* registry {};

    wl_surface* surface {};
    xdg_surface* xdgSurface {};
    xdg_toplevel* xdgToplevel {};
    wl_output* output {};

    wl_egl_window* eglWindow {};
    EGLDisplay eglDisplay {};
    EGLContext eglContext {};
    EGLSurface eglSurface {};

    wl_seat* seat {};
    wl_shm* shm {};
    wl_compositor* compositor {};
    xdg_wm_base* xdgWmBase {};
    [[maybe_unused]] u32 xdgConfigureSerial = 0;

    wl_pointer* pointer {};
    wl_surface* cursorSurface {};
    wl_cursor_image* cursorImage {};
    wl_cursor_theme* cursorTheme {};

    u32 _pointerSerial = 0;
    zwp_pointer_constraints_v1* pointerConstraints {};
    zwp_locked_pointer_v1* lockedPointer {};
    zwp_confined_pointer_v1* confinedPointer {};
    zwp_relative_pointer_v1* relativePointer {};
    zwp_relative_pointer_manager_v1* relativePointerManager {};

    wl_keyboard* keyboard {};

    bool bRestoreRelativeMode = false;

    Client() = default;
    Client(String name);
};

void ClientDestroy(Client* s);
void ClientInit(Client* s);
void ClientEnableRelativeMode(Client* s);
void ClientDisableRelativeMode(Client* s);
void ClientHideCursor(Client* s);
void ClientSetCursorImage(Client* s, String cursorType);
void ClientSetFullscreen(Client* s);
void ClientUnsetFullscreen(Client* s);
void ClientTogglePointerRelativeMode(Client* s);
void ClientToggleFullscreen(Client* s);
void ClientBindGlContext(Client* s);
void ClientUnbindGlContext(Client* s);
void ClientSetSwapInterval(Client* s, int interval);
void ClientToggleVSync(Client* s);
void ClientSwapBuffers(Client* s);
void ClientProcEvents(Client* s);
void ClientShowWindow(Client* s);

} /* namespace wayland */
} /* namespace platform */

inline void WindowInit(platform::wayland::Client* s) { ClientInit(s); }
inline void WindowDisableRelativeMode(platform::wayland::Client* s) { ClientDisableRelativeMode(s); }
inline void WindowEnableRelativeMode(platform::wayland::Client* s) { ClientEnableRelativeMode(s); }
inline void WindowTogglePointerRelativeMode(platform::wayland::Client* s) { ClientTogglePointerRelativeMode(s); }
inline void WindowToggleFullscreen(platform::wayland::Client* s) { ClientToggleFullscreen(s); }
inline void WindowHideCursor(platform::wayland::Client* s) { ClientHideCursor(s); }
inline void WindowSetCursorImage(platform::wayland::Client* s, String cursorType) { ClientSetCursorImage(s, cursorType); }
inline void WindowSetFullscreen(platform::wayland::Client* s) { ClientSetFullscreen(s); }
inline void WindowUnsetFullscreen(platform::wayland::Client* s) { ClientUnsetFullscreen(s); }
inline void WindowBindGlContext(platform::wayland::Client* s) { ClientBindGlContext(s); }
inline void WindowUnbindGlContext(platform::wayland::Client* s) { ClientUnbindGlContext(s); }
inline void WindowSetSwapInterval(platform::wayland::Client* c, int interval) { ClientSetSwapInterval(c, interval); }
inline void WindowToggleVSync(platform::wayland::Client* s) { ClientToggleVSync(s); }
inline void WindowSwapBuffers(platform::wayland::Client* s) { ClientSwapBuffers(s); }
inline void WindowProcEvents(platform::wayland::Client* s) { ClientProcEvents(s); }
inline void WindowShowWindow(platform::wayland::Client* s) { ClientShowWindow(s); }
inline void WindowDestroy(platform::wayland::Client* s) { ClientDestroy(s); }
