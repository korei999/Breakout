#pragma once

#include "IWindow.hh"

#include <X11/Xlib.h>
#include <EGL/egl.h>

namespace platform
{
namespace x11
{

struct Win : IWindow
{
    Display* m_pDisplay {};
    ::Window m_window {};
    EGLDisplay m_pEGLDisplay {};
    EGLSurface m_pEGLSurface {};
    EGLContext m_pEGLContext {};

    /* */

    Win(String sName) : IWindow(sName) {}

    /* */

    virtual void start() override final;
    virtual void disableRelativeMode() override final;
    virtual void enableRelativeMode() override final;
    virtual void togglePointerRelativeMode() override final;
    virtual void toggleFullscreen() override final;
    virtual void hideCursor() override final;
    virtual void setCursorImage(String cursorType) override final;
    virtual void setFullscreen() override final;
    virtual void unsetFullscreen() override final;
    virtual void bindGlContext() override final;
    virtual void unbindGlContext() override final;
    virtual void setSwapInterval(int interval) override final;
    virtual void toggleVSync() override final;
    virtual void swapBuffers() override final;
    virtual void procEvents() override final;
    virtual void showWindow() override final;
    virtual void destroy() override final;

    /* */

    void runMouseReadingThread();
    void mapX11KeycodesToLinuxKeycodes();
    void storeAtoms();
};

} /* namespace x11 */
} /* namespace platform */
