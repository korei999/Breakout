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
    HINSTANCE _hInstance;
    HWND hWindow;
    HDC hDeviceContext;
    HGLRC hGlContext;
    WNDCLASSEXW windowClass;
    RAWINPUTDEVICE rawInputDevices[2];

    Win32Window(String sName, HINSTANCE hInstance) : IWindow(sName), _hInstance(hInstance) {}

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

} /* namespace win32 */
} /* namespace platform */
