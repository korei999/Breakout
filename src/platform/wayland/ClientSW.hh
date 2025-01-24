#pragma once

#include "IWindow.hh"

#include <wayland-client.h>

namespace platform::wayland
{

struct ClientSW final : public IWindow
{
    wl_display* m_pDisplay {};

    ClientSW(String sName) : IWindow(sName) {}

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
};

} /* namespace platform::wayland */
