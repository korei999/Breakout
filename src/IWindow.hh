#pragma once

#include "adt/String.hh"

using namespace adt;

/* Platform abstracted application/window interface */
struct IWindow;

struct IWindow
{
    String sName {};
    int wWidth = 1920;
    int wHeight = 1080;
    bool bRunning = false;
    bool bConfigured = false;
    bool bPaused = false;
    bool bPointerRelativeMode = false;
    bool bHideCursor = false;
    bool bFullscreen = false;
    int swapInterval = 1;
    f64 hideCursorTime = 0.0f;

    constexpr IWindow() = default;
    constexpr IWindow(String _sName = "Breakout")
        : sName(_sName) {}

    virtual void start() = 0;
    virtual void disableRelativeMode() = 0;
    virtual void enableRelativeMode() = 0;
    virtual void togglePointerRelativeMode() = 0;
    virtual void toggleFullscreen() = 0;
    virtual void hideCursor() = 0;
    virtual void setCursorImage(String cursorType) = 0;
    virtual void setFullscreen() = 0;
    virtual void unsetFullscreen() = 0;
    virtual void bindGlContext() = 0;
    virtual void unbindGlContext() = 0;
    virtual void setSwapInterval(int interval) = 0;
    virtual void toggleVSync() = 0;
    virtual void swapBuffers() = 0;
    virtual void procEvents() = 0;
    virtual void showWindow() = 0;
    virtual void destroy() = 0;
};
