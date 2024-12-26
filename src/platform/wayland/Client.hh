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
    wl_display* m_display {};
    wl_registry* m_registry {};

    wl_surface* m_surface {};
    xdg_surface* m_xdgSurface {};
    xdg_toplevel* m_xdgToplevel {};
    wl_output* m_output {};

    wl_egl_window* m_eglWindow {};
    EGLDisplay m_eglDisplay {};
    EGLContext m_eglContext {};
    EGLSurface m_eglSurface {};

    wl_seat* m_seat {};
    wl_shm* m_shm {};
    wl_compositor* m_compositor {};
    xdg_wm_base* m_xdgWmBase {};
    [[maybe_unused]] u32 m_xdgConfigureSerial = 0;

    wl_pointer* m_pointer {};
    wl_surface* m_cursorSurface {};
    wl_cursor_image* m_cursorImage {};
    wl_cursor_theme* m_cursorTheme {};

    u32 m_pointerSerial = 0;
    zwp_pointer_constraints_v1* m_pointerConstraints {};
    zwp_locked_pointer_v1* m_lockedPointer {};
    zwp_confined_pointer_v1* m_confinedPointer {};
    zwp_relative_pointer_v1* m_relativePointer {};
    zwp_relative_pointer_manager_v1* m_relativePointerManager {};

    wl_keyboard* m_keyboard {};

    bool m_bRestoreRelativeMode = false;
    bool m_bConfigured = false;

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
