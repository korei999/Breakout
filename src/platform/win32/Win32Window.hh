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

struct Win32Window : public IWindow
{
    HINSTANCE m_hInstance;
    HWND m_hWindow;
    HDC m_hDeviceContext;
    HGLRC m_hGlContext;
    WNDCLASSEXW m_windowClass;
    RAWINPUTDEVICE m_rawInputDevices[2];

    Win32Window(String sName, HINSTANCE hInstance) : IWindow(sName), m_hInstance(hInstance) {}

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

} /* namespace win32 */
} /* namespace platform */
