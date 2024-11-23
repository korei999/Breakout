#pragma once

#include "adt/String.hh"

using namespace adt;

/* Platform abstracted application/window interface */
struct WindowAbstract;

struct WindowInterface
{
    void (*start)(WindowAbstract*);
    void (*disableRelativeMode)(WindowAbstract*);
    void (*enableRelativeMode)(WindowAbstract*);
    void (*togglePointerRelativeMode)(WindowAbstract*);
    void (*toggleFullscreen)(WindowAbstract*);
    void (*hideCursor)(WindowAbstract*);
    void (*setCursorImage)(WindowAbstract*, String cursorType);
    void (*setFullscreen)(WindowAbstract*);
    void (*unsetFullscreen)(WindowAbstract*);
    void (*bindGlContext)(WindowAbstract*);
    void (*unbindGlContext)(WindowAbstract*);
    void (*setSwapInterval)(WindowAbstract*, int interval);
    void (*toggleVSync)(WindowAbstract*);
    void (*swapBuffers)(WindowAbstract*);
    void (*procEvents)(WindowAbstract*);
    void (*showWindow)(WindowAbstract*);
    void (*destroy)(WindowAbstract*);
};

struct WindowAbstract /* x11 window is also a 'Window' */
{
    const WindowInterface* pVTable {};
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

    constexpr WindowAbstract() = default;
    constexpr WindowAbstract(WindowInterface* _pVTable, String _sName = "Breakout")
        : pVTable(_pVTable), sName(_sName) {}
};

ADT_NO_UB constexpr void WindowStart(WindowAbstract* s) { s->pVTable->start(s); }
ADT_NO_UB constexpr void WindowDisableRelativeMode(WindowAbstract* s) { s->pVTable->disableRelativeMode(s); }
ADT_NO_UB constexpr void WindowEnableRelativeMode(WindowAbstract* s) { s->pVTable->enableRelativeMode(s); }
ADT_NO_UB constexpr void WindowTogglePointerRelativeMode(WindowAbstract* s) { s->pVTable->togglePointerRelativeMode(s); }
ADT_NO_UB constexpr void WindowToggleFullscreen(WindowAbstract* s) { s->pVTable->toggleFullscreen(s); }
ADT_NO_UB constexpr void WindowHideCursor(WindowAbstract* s) { s->pVTable->hideCursor(s); }
ADT_NO_UB constexpr void WindowSetCursorImage(WindowAbstract* s, String cursorType) { s->pVTable->setCursorImage(s, cursorType); }
ADT_NO_UB constexpr void WindowSetFullscreen(WindowAbstract* s) { s->pVTable->setFullscreen(s); }
ADT_NO_UB constexpr void WindowUnsetFullscreen(WindowAbstract* s) { s->pVTable->unsetFullscreen(s); }
ADT_NO_UB constexpr void WindowBindGlContext(WindowAbstract* s) { s->pVTable->bindGlContext(s); }
ADT_NO_UB constexpr void WindowUnbindGlContext(WindowAbstract* s) { s->pVTable->unbindGlContext(s); }
ADT_NO_UB constexpr void WindowSetSwapInterval(WindowAbstract* c, int interval) { c->pVTable->setSwapInterval(c, interval); }
ADT_NO_UB constexpr void WindowToggleVSync(WindowAbstract* s) { s->pVTable->toggleVSync(s); }
ADT_NO_UB constexpr void WindowSwapBuffers(WindowAbstract* s) { s->pVTable->swapBuffers(s); }
ADT_NO_UB constexpr void WindowProcEvents(WindowAbstract* s) { s->pVTable->procEvents(s); }
ADT_NO_UB constexpr void WindowShowWindow(WindowAbstract* s) { s->pVTable->showWindow(s); }
ADT_NO_UB constexpr void WindowDestroy(WindowAbstract* s) { s->pVTable->destroy(s); }
