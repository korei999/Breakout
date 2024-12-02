#pragma once

#include "adt/String.hh"

using namespace adt;

/* Platform abstracted application/window interface */
struct IWindow;

struct WindowVTable
{
    void (*start)(IWindow*);
    void (*disableRelativeMode)(IWindow*);
    void (*enableRelativeMode)(IWindow*);
    void (*togglePointerRelativeMode)(IWindow*);
    void (*toggleFullscreen)(IWindow*);
    void (*hideCursor)(IWindow*);
    void (*setCursorImage)(IWindow*, String cursorType);
    void (*setFullscreen)(IWindow*);
    void (*unsetFullscreen)(IWindow*);
    void (*bindGlContext)(IWindow*);
    void (*unbindGlContext)(IWindow*);
    void (*setSwapInterval)(IWindow*, int interval);
    void (*toggleVSync)(IWindow*);
    void (*swapBuffers)(IWindow*);
    void (*procEvents)(IWindow*);
    void (*showWindow)(IWindow*);
    void (*destroy)(IWindow*);
};

struct IWindow
{
    const WindowVTable* pVTable {};
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
    constexpr IWindow(WindowVTable* _pVTable, String _sName = "Breakout")
        : pVTable(_pVTable), sName(_sName) {}
};

ADT_NO_UB constexpr void WindowStart(IWindow* s) { s->pVTable->start(s); }
ADT_NO_UB constexpr void WindowDisableRelativeMode(IWindow* s) { s->pVTable->disableRelativeMode(s); }
ADT_NO_UB constexpr void WindowEnableRelativeMode(IWindow* s) { s->pVTable->enableRelativeMode(s); }
ADT_NO_UB constexpr void WindowTogglePointerRelativeMode(IWindow* s) { s->pVTable->togglePointerRelativeMode(s); }
ADT_NO_UB constexpr void WindowToggleFullscreen(IWindow* s) { s->pVTable->toggleFullscreen(s); }
ADT_NO_UB constexpr void WindowHideCursor(IWindow* s) { s->pVTable->hideCursor(s); }
ADT_NO_UB constexpr void WindowSetCursorImage(IWindow* s, String cursorType) { s->pVTable->setCursorImage(s, cursorType); }
ADT_NO_UB constexpr void WindowSetFullscreen(IWindow* s) { s->pVTable->setFullscreen(s); }
ADT_NO_UB constexpr void WindowUnsetFullscreen(IWindow* s) { s->pVTable->unsetFullscreen(s); }
ADT_NO_UB constexpr void WindowBindGlContext(IWindow* s) { s->pVTable->bindGlContext(s); }
ADT_NO_UB constexpr void WindowUnbindGlContext(IWindow* s) { s->pVTable->unbindGlContext(s); }
ADT_NO_UB constexpr void WindowSetSwapInterval(IWindow* c, int interval) { c->pVTable->setSwapInterval(c, interval); }
ADT_NO_UB constexpr void WindowToggleVSync(IWindow* s) { s->pVTable->toggleVSync(s); }
ADT_NO_UB constexpr void WindowSwapBuffers(IWindow* s) { s->pVTable->swapBuffers(s); }
ADT_NO_UB constexpr void WindowProcEvents(IWindow* s) { s->pVTable->procEvents(s); }
ADT_NO_UB constexpr void WindowShowWindow(IWindow* s) { s->pVTable->showWindow(s); }
ADT_NO_UB constexpr void WindowDestroy(IWindow* s) { s->pVTable->destroy(s); }
