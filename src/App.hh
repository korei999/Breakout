#pragma once

#include "adt/String.hh"

using namespace adt;

/* Platform abstracted application/window interface */
struct App;

struct AppInterface
{
    void (*init)(App*);
    void (*disableRelativeMode)(App*);
    void (*enableRelativeMode)(App*);
    void (*togglePointerRelativeMode)(App*);
    void (*toggleFullscreen)(App*);
    void (*hideCursor)(App*);
    void (*setCursorImage)(App*, String cursorType);
    void (*setFullscreen)(App*);
    void (*unsetFullscreen)(App*);
    void (*bindGlContext)(App*);
    void (*unbindGlContext)(App*);
    void (*setSwapInterval)(App*, int interval);
    void (*toggleVSync)(App*);
    void (*swapBuffers)(App*);
    void (*procEvents)(App*);
    void (*showWindow)(App*);
    void (*destroy)(App*);
};

struct App
{
    const AppInterface* pVTable;
    String sName;
    int wWidth = 1920;
    int wHeight = 1080;
    bool bRunning = false;
    bool bConfigured = false;
    int swapInterval = 1;
    bool bPaused = false;
    bool bPointerRelativeMode = false;
    bool bFullscreen = false;
};

ADT_NO_UB constexpr void AppInit(App* s) { s->pVTable->init(s); }
ADT_NO_UB constexpr void AppDisableRelativeMode(App* s) { s->pVTable->disableRelativeMode(s); }
ADT_NO_UB constexpr void AppEnableRelativeMode(App* s) { s->pVTable->enableRelativeMode(s); }
ADT_NO_UB constexpr void AppTogglePointerRelativeMode(App* s) { s->pVTable->togglePointerRelativeMode(s); }
ADT_NO_UB constexpr void AppToggleFullscreen(App* s) { s->pVTable->toggleFullscreen(s); }
ADT_NO_UB constexpr void AppHideCursor(App* s) { s->pVTable->hideCursor(s); }
ADT_NO_UB constexpr void AppSetCursorImage(App* s, String cursorType) { s->pVTable->setCursorImage(s, cursorType); }
ADT_NO_UB constexpr void AppSetFullscreen(App* s) { s->pVTable->setFullscreen(s); }
ADT_NO_UB constexpr void AppUnsetFullscreen(App* s) { s->pVTable->unsetFullscreen(s); }
ADT_NO_UB constexpr void AppBindGlContext(App* s) { s->pVTable->bindGlContext(s); }
ADT_NO_UB constexpr void AppUnbindGlContext(App* s) { s->pVTable->unbindGlContext(s); }
ADT_NO_UB constexpr void AppSetSwapInterval(App* c, int interval) { c->pVTable->setSwapInterval(c, interval); }
ADT_NO_UB constexpr void AppToggleVSync(App* s) { s->pVTable->toggleVSync(s); }
ADT_NO_UB constexpr void AppSwapBuffers(App* s) { s->pVTable->swapBuffers(s); }
ADT_NO_UB constexpr void AppProcEvents(App* s) { s->pVTable->procEvents(s); }
ADT_NO_UB constexpr void AppShowWindow(App* s) { s->pVTable->showWindow(s); }
ADT_NO_UB constexpr void AppDestroy(App* s) { s->pVTable->destroy(s); }
