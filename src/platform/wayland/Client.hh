#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl-core.h>

#include "wayland-protocols/pointer-constraints-unstable-v1.h"
#include "wayland-protocols/relative-pointer-unstable-v1.h"
#include "wayland-protocols/xdg-shell.h"

#include "IWindow.hh"

#include "adt/types.hh"

using namespace adt;

namespace platform
{
namespace wayland
{

struct Client : IWindow
{
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

    Client(String name) : IWindow(name) {}

    virtual void start();
    virtual void disableRelativeMode();
    virtual void enableRelativeMode();
    virtual void togglePointerRelativeMode();
    virtual void toggleFullscreen();
    virtual void hideCursor();
    virtual void setCursorImage(String cursorType);
    virtual void setFullscreen();
    virtual void unsetFullscreen();
    virtual void bindGlContext();
    virtual void unbindGlContext();
    virtual void setSwapInterval(int interval);
    virtual void toggleVSync();
    virtual void swapBuffers();
    virtual void procEvents();
    virtual void showWindow();
    virtual void destroy();
};

} /* namespace wayland */
} /* namespace platform */
