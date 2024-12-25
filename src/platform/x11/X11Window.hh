#pragma once

#include "IWindow.hh"

#include <X11/Xlib.h>
#include <EGL/egl.h>

namespace platform
{
namespace x11
{

struct Window : IWindow
{
    Display* pDisplay {};
    ::Window window {};
    EGLDisplay pEGLDisplay {};
    EGLSurface pEGLSurface {};
    EGLContext pEGLContext {};

    Window(String sName) : IWindow(sName) {}

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

} /* namespace x11 */
} /* namespace platform */
