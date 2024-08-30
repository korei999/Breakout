#pragma once

#include "adt/String.hh"

using namespace adt;

struct App;

struct AppInterface
{
    void (*init)(App*);
    void (*disableRelativeMode)(App*);
    void (*enableRelativeMode)(App*);
    void (*togglePointerRelativeMode)(App*);
    void (*toggleFullscreen)(App*);
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
    bool bRelativeMode = false;
    bool bFullscreen = false;
};

ADT_NO_UB constexpr void AppInit(App* c) { c->pVTable->init(c); }
ADT_NO_UB constexpr void AppDisableRelativeMode(App* c) { c->pVTable->disableRelativeMode(c); }
ADT_NO_UB constexpr void AppEnableRelativeMode(App* c) { c->pVTable->enableRelativeMode(c); }
ADT_NO_UB constexpr void AppTogglePointerRelativeMode(App* c) { c->pVTable->togglePointerRelativeMode(c); }
ADT_NO_UB constexpr void AppToggleFullscreen(App* c) { c->pVTable->toggleFullscreen(c); }
ADT_NO_UB constexpr void AppSetCursorImage(App* c, String cursorType) { c->pVTable->setCursorImage(c, cursorType); }
ADT_NO_UB constexpr void AppSetFullscreen(App* c) { c->pVTable->setFullscreen(c); }
ADT_NO_UB constexpr void AppUnsetFullscreen(App* c) { c->pVTable->unsetFullscreen(c); }
ADT_NO_UB constexpr void AppBindGlContext(App* c) { c->pVTable->bindGlContext(c); }
ADT_NO_UB constexpr void AppUnbindGlContext(App* c) { c->pVTable->unbindGlContext(c); }
ADT_NO_UB constexpr void AppSetSwapInterval(App* c, int interval) { c->pVTable->setSwapInterval(c, interval); }
ADT_NO_UB constexpr void AppToggleVSync(App* c) { c->pVTable->toggleVSync(c); }
ADT_NO_UB constexpr void AppSwapBuffers(App* c) { c->pVTable->swapBuffers(c); }
ADT_NO_UB constexpr void AppProcEvents(App* c) { c->pVTable->procEvents(c); }
ADT_NO_UB constexpr void AppShowWindow(App* c) { c->pVTable->showWindow(c); }
ADT_NO_UB constexpr void AppDestroy(App* c) { c->pVTable->destroy(c); }
