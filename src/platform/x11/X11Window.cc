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
Window::start()
{
    this->pDisplay = XOpenDisplay({});
    if (!this->pDisplay)
    {
        CERR("XOpenDisplay(): failed\n");
        exit(1);
    }

    XSetWindowAttributes attrs {};
    attrs.event_mask = StructureNotifyMask;

    this->window = XCreateWindow(
        this->pDisplay,
        DefaultRootWindow(this->pDisplay),
        0, 0, this->m_wWidth, this->m_wHeight,
        0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
        &attrs
    );

    assert(this->window && "XCreateWindow()");

    {
        this->pEGLDisplay = eglGetDisplay((EGLNativeDisplayType)this->pDisplay);
        assert(this->pEGLDisplay && "eglGetDisplay()");

        EGLint major, minor;
        if (!eglInitialize(this->pEGLDisplay, &major, &minor))
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
        if (!eglChooseConfig(this->pEGLDisplay, aAttr, &config, 1, &count) || count != 1)
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

        this->pEGLSurface = eglCreateWindowSurface(this->pEGLDisplay, config, this->window, aAttr);
        if (this->pEGLSurface == EGL_NO_SURFACE)
        {
            CERR("this->pEGLSurface == EGL_NO_SURFACE\n");
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

        this->pEGLContext =eglCreateContext(this->pEGLDisplay, config, EGL_NO_CONTEXT, aAttr);
        if (this->pEGLContext == EGL_NO_CONTEXT)
        {
            CERR("context == EGL_NO_CONTEXT\n");
            exit(1);
        }
    }

    XStoreName(this->pDisplay, this->window, this->m_sName.data());

    XSelectInput(
        this->pDisplay,
        this->window,
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

    input::storeAtoms(this);
    input::mapX11KeycodesToLinuxKeycodes(this);
}

void
Window::destroy()
{
    if (this->pEGLDisplay) eglTerminate(this->pEGLDisplay);
    XDestroyWindow(this->pDisplay, this->window);
    if (this->pDisplay) XCloseDisplay(this->pDisplay);
}

void
Window::enableRelativeMode()
{
}

void
Window::disableRelativeMode()
{
}

void
Window::hideCursor()
{
}

void
Window::setCursorImage(String cursorType)
{
}

void
Window::setFullscreen()
{
    if (this->m_bFullscreen) return;

    this->m_bFullscreen = true;
    Atom atomWmState = XInternAtom(this->pDisplay, "_NET_WM_STATE", true);
    Atom atomWmFullscreen = XInternAtom(this->pDisplay, "_NET_WM_STATE_FULLSCREEN", true);

    XChangeProperty(this->pDisplay, this->window, atomWmState, XA_ATOM, 32, PropModeReplace, (unsigned char*)&atomWmFullscreen, 1);
}

void
Window::unsetFullscreen()
{
    this->m_bFullscreen = false;
}

void
Window::togglePointerRelativeMode()
{
}

void
Window::toggleFullscreen()
{
    if (this->m_bFullscreen) unsetFullscreen();
    else setFullscreen();

    LOG_NOTIFY("bFullscreen: {}\n", this->m_bFullscreen);
}

void
Window::bindGlContext()
{
    EGLD ( eglMakeCurrent(this->pEGLDisplay, this->pEGLSurface, this->pEGLSurface, this->pEGLContext) );
}

void
Window::unbindGlContext()
{
    EGLD( eglMakeCurrent(this->pEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
Window::setSwapInterval(int interval)
{
    this->m_swapInterval = interval;
    EGLD( eglSwapInterval(this->pEGLDisplay, interval) );
}

void
Window::toggleVSync()
{
    this->m_swapInterval = !this->m_swapInterval;
    EGLD( eglSwapInterval(this->pEGLDisplay, this->m_swapInterval) );
    LOG_OK("swapInterval: {}\n", this->m_swapInterval);
}

void
Window::swapBuffers()
{
    EGLD( eglSwapBuffers(this->pEGLDisplay, this->pEGLSurface) );
}

void
Window::procEvents()
{
    input::procEvents(this);
}

void
Window::showWindow()
{
    XMapRaised(this->pDisplay, this->window);
}

} /* namespace x11 */
} /* namespace platform */
