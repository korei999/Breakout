#pragma once

#include "App.hh"

#include <windef.h>

namespace win32
{

struct Window : App
{
    HINSTANCE _hInstance;
    HWND _hWindow;
    HDC _hDeviceContext;
    HGLRC _hGlContext;
    WNDCLASSEXW _windowClass;
    RAWINPUTDEVICE _rawInputDevices[2];

    Window(Allocator* p, String name, HINSTANCE instance);
    virtual ~Window() override;

    virtual void init() override;
    virtual void disableRelativeMode() override;
    virtual void enableRelativeMode() override;
    virtual void togglePointerRelativeMode() override;
    virtual void toggleFullscreen() override;
    virtual void setCursorImage(String cursorType) override;
    virtual void setFullscreen() override;
    virtual void unsetFullscreen() override;
    virtual void bindGlContext() override;
    virtual void unbindGlContext() override;
    virtual void setSwapInterval(int interval) override;
    virtual void toggleVSync() override;
    virtual void swapBuffers() override;
    virtual void procEvents() override;
    virtual void showWindow() override;
};

};
