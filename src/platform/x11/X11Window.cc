#include "X11Window.hh"

#include "adt/logs.hh"
#include "input.hh"

#include <X11/Xatom.h>

namespace platform
{
namespace x11
{

/* https://gist.github.com/mmozeiko/911347b5e3d998621295794e0ba334c4 */

EGLint eglLastErrorCode = EGL_SUCCESS;

#ifndef NDEBUG
#    define EGLD(C)                                                                                                    \
        {                                                                                                              \
            C;                                                                                                         \
            if ((eglLastErrorCode = eglGetError()) != EGL_SUCCESS)                                                     \
                LOG_FATAL("eglLastErrorCode: {:#x}\n", eglLastErrorCode);                                              \
        }
#else
#    define EGLD(C) C
#endif

void
WindowStart(Window* s)
{
    s->pDisplay = XOpenDisplay({});
    if (!s->pDisplay)
    {
        CERR("XOpenDisplay(): failed\n");
        exit(1);
    }

    XSetWindowAttributes attrs {};
    attrs.event_mask = StructureNotifyMask;

    s->window = XCreateWindow(
        s->pDisplay,
        DefaultRootWindow(s->pDisplay),
        0, 0, s->super.wWidth, s->super.wHeight,
        0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
        &attrs
    );

    assert(s->window && "XCreateWindow()");

    {
        s->pEGLDisplay = eglGetDisplay((EGLNativeDisplayType)s->pDisplay);
        assert(s->pEGLDisplay && "eglGetDisplay()");

        EGLint major, minor;
        if (!eglInitialize(s->pEGLDisplay, &major, &minor))
        {
            CERR("eglInitialize(): failed\n");
            exit(1);
        }
        if (major < 1 || (major == 1 && minor < 5))
        {
            CERR("EGL version 1.5 or higher required\n");
            exit(1);
        }
    }

    EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
    assert(ok && "eglBindAPI()");

    EGLConfig config {};
    {
        EGLint aAttr[] {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_CONFORMANT, EGL_OPENGL_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
        
            // uncomment for multisampled framebuffer
            // EGL_SAMPLE_BUFFERS, 1,
            // EGL_SAMPLES,        4, /* 4x MSAA */
        
            EGL_NONE,
        };

        EGLint count {};
        if (!eglChooseConfig(s->pEGLDisplay, aAttr, &config, 1, &count) || count != 1)
        {
            CERR("eglChooseConfig(): failed\n");
            exit(1);
        }
    }

    {
        EGLint aAttr[] {
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR, /* or use EGL_GL_COLORSPACE_SRGB for sRGB framebuffer */
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        };

        s->pEGLSurface = eglCreateWindowSurface(s->pEGLDisplay, config, s->window, aAttr);
        if (s->pEGLSurface == EGL_NO_SURFACE)
        {
            CERR("s->pEGLSurface == EGL_NO_SURFACE\n");
            exit(1);
        }
    }

    {
        EGLint aAttr[] {
            EGL_CONTEXT_MAJOR_VERSION, 4,
            EGL_CONTEXT_MINOR_VERSION, 5,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#ifndef NDEBUG
            EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
            EGL_NONE,
        };

        s->pEGLContext =eglCreateContext(s->pEGLDisplay, config, EGL_NO_CONTEXT, aAttr);
        if (s->pEGLContext == EGL_NO_CONTEXT)
        {
            CERR("context == EGL_NO_CONTEXT\n");
            exit(1);
        }
    }

    XStoreName(s->pDisplay, s->window, s->super.sName.pData);

    XSelectInput(
        s->pDisplay,
        s->window,
        KeyPressMask|
        KeyReleaseMask|
        ButtonPressMask|
        ButtonReleaseMask|
        PointerMotionMask|
        EnterWindowMask|
        LeaveWindowMask|
        FocusChangeMask|
        StructureNotifyMask
    );

    input::storeAtoms(s);
    input::mapX11KeycodesToLinuxKeycodes(s);
}

void
WindowDestroy(Window* s)
{
    if (s->pEGLDisplay) eglTerminate(s->pEGLDisplay);
    XDestroyWindow(s->pDisplay, s->window);
    if (s->pDisplay) XCloseDisplay(s->pDisplay);
}

void
WindowEnableRelativeMode(Window* s)
{
}

void
WindowDisableRelativeMode(Window* s)
{
}

void
WindowHideCursor(Window* s)
{
}

void
WindowSetCursorImage(Window* s, String cursorType)
{
}

void
WindowSetFullscreen(Window* s)
{
    // s->super.bFullscreen = true;
    // Atom atomWmState = XInternAtom(s->pDisplay, "_NET_WM_STATE", true);
    // Atom atomWmFullscreen = XInternAtom(s->pDisplay, "_NET_WM_STATE_FULLSCREEN", true);

    // XChangeProperty(s->pDisplay, s->window, atomWmState, XA_ATOM, 32, PropModeReplace, (unsigned char*)&atomWmFullscreen, 1);
}

void
WindowUnsetFullscreen(Window* s)
{
}

void
WindowTogglePointerRelativeMode(Window* s)
{
}

void
WindowToggleFullscreen(Window* s)
{
}

void
WindowBindGlContext(Window* s)
{
    EGLD ( eglMakeCurrent(s->pEGLDisplay, s->pEGLSurface, s->pEGLSurface, s->pEGLContext) );
}

void
WindowUnbindGlContext(Window* s)
{
    EGLD( eglMakeCurrent(s->pEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
WindowSetSwapInterval(Window* s, int interval)
{
    s->super.swapInterval = interval;
    EGLD( eglSwapInterval(s->pEGLDisplay, interval) );
}

void
WindowToggleVSync(Window* s)
{
    s->super.swapInterval = !s->super.swapInterval;
    EGLD( eglSwapInterval(s->pEGLDisplay, s->super.swapInterval) );
    LOG_OK("swapInterval: {}\n", s->super.swapInterval);
}

void
WindowSwapBuffers(Window* s)
{
    EGLD( eglSwapBuffers(s->pEGLDisplay, s->pEGLSurface) );
}

void
WindowProcEvents(Window* s)
{
    input::procEvents(s);
}

void
WindowShowWindow(Window* s)
{
    XMapRaised(s->pDisplay, s->window);
}

Window::Window(String sName)
{
    static WindowInterface s_vTable {
        .start = (decltype(WindowInterface::start))WindowStart,
        .disableRelativeMode = (decltype(WindowInterface::disableRelativeMode))WindowDisableRelativeMode,
        .enableRelativeMode = (decltype(WindowInterface::enableRelativeMode))WindowEnableRelativeMode,
        .togglePointerRelativeMode = (decltype(WindowInterface::togglePointerRelativeMode))WindowTogglePointerRelativeMode,
        .toggleFullscreen = (decltype(WindowInterface::toggleFullscreen))WindowToggleFullscreen,
        .hideCursor = (decltype(WindowInterface::hideCursor))WindowHideCursor,
        .setCursorImage = (decltype(WindowInterface::setCursorImage))WindowSetCursorImage,
        .setFullscreen = (decltype(WindowInterface::setFullscreen))WindowSetFullscreen,
        .unsetFullscreen = (decltype(WindowInterface::unsetFullscreen))WindowUnsetFullscreen,
        .bindGlContext = (decltype(WindowInterface::bindGlContext))WindowBindGlContext,
        .unbindGlContext = (decltype(WindowInterface::unbindGlContext))WindowUnbindGlContext,
        .setSwapInterval = (decltype(WindowInterface::setSwapInterval))WindowSetSwapInterval,
        .toggleVSync = (decltype(WindowInterface::toggleVSync))WindowToggleVSync,
        .swapBuffers = (decltype(WindowInterface::swapBuffers))WindowSwapBuffers,
        .procEvents = (decltype(WindowInterface::procEvents))WindowProcEvents,
        .showWindow = (decltype(WindowInterface::showWindow))WindowShowWindow,
        .destroy = (decltype(WindowInterface::destroy))WindowDestroy,
    };

    this->super.sName = sName;
    this->super.pVTable = &s_vTable;
}

} /* namespace x11 */
} /* namespace platform */
