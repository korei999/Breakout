#pragma once

#include "adt/String.hh"

using namespace adt;

/* Platform abstracted application/window interface */
struct Window;

struct WindowInterface
{
    void (*init)(Window*);
    void (*disableRelativeMode)(Window*);
    void (*enableRelativeMode)(Window*);
    void (*togglePointerRelativeMode)(Window*);
    void (*toggleFullscreen)(Window*);
    void (*hideCursor)(Window*);
    void (*setCursorImage)(Window*, String cursorType);
    void (*setFullscreen)(Window*);
    void (*unsetFullscreen)(Window*);
    void (*bindGlContext)(Window*);
    void (*unbindGlContext)(Window*);
    void (*setSwapInterval)(Window*, int interval);
    void (*toggleVSync)(Window*);
    void (*swapBuffers)(Window*);
    void (*procEvents)(Window*);
    void (*showWindow)(Window*);
    void (*destroy)(Window*);
};

struct Window
{
    const WindowInterface* pVTable;
    String sName;
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
};

ADT_NO_UB constexpr void WindowInit(Window* s) { s->pVTable->init(s); }
ADT_NO_UB constexpr void WindowDisableRelativeMode(Window* s) { s->pVTable->disableRelativeMode(s); }
ADT_NO_UB constexpr void WindowEnableRelativeMode(Window* s) { s->pVTable->enableRelativeMode(s); }
ADT_NO_UB constexpr void WindowTogglePointerRelativeMode(Window* s) { s->pVTable->togglePointerRelativeMode(s); }
ADT_NO_UB constexpr void WindowToggleFullscreen(Window* s) { s->pVTable->toggleFullscreen(s); }
ADT_NO_UB constexpr void WindowHideCursor(Window* s) { s->pVTable->hideCursor(s); }
ADT_NO_UB constexpr void WindowSetCursorImage(Window* s, String cursorType) { s->pVTable->setCursorImage(s, cursorType); }
ADT_NO_UB constexpr void WindowSetFullscreen(Window* s) { s->pVTable->setFullscreen(s); }
ADT_NO_UB constexpr void WindowUnsetFullscreen(Window* s) { s->pVTable->unsetFullscreen(s); }
ADT_NO_UB constexpr void WindowBindGlContext(Window* s) { s->pVTable->bindGlContext(s); }
ADT_NO_UB constexpr void WindowUnbindGlContext(Window* s) { s->pVTable->unbindGlContext(s); }
ADT_NO_UB constexpr void WindowSetSwapInterval(Window* c, int interval) { c->pVTable->setSwapInterval(c, interval); }
ADT_NO_UB constexpr void WindowToggleVSync(Window* s) { s->pVTable->toggleVSync(s); }
ADT_NO_UB constexpr void WindowSwapBuffers(Window* s) { s->pVTable->swapBuffers(s); }
ADT_NO_UB constexpr void WindowProcEvents(Window* s) { s->pVTable->procEvents(s); }
ADT_NO_UB constexpr void WindowShowWindow(Window* s) { s->pVTable->showWindow(s); }
ADT_NO_UB constexpr void WindowDestroy(Window* s) { s->pVTable->destroy(s); }
