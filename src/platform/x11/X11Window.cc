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
Win::start()
{
    m_pDisplay = XOpenDisplay({});
    if (!m_pDisplay)
    {
        CERR("XOpenDisplay(): failed\n");
        exit(1);
    }

    XSetWindowAttributes attrs {};
    attrs.event_mask = StructureNotifyMask;
    attrs.override_redirect = true;

    m_window = XCreateWindow(
        m_pDisplay,
        DefaultRootWindow(m_pDisplay),
        0, 0, m_wWidth, m_wHeight,
        0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
        &attrs
    );

    assert(m_window && "XCreateWindow()");

    {
        m_pEGLDisplay = eglGetDisplay((EGLNativeDisplayType)m_pDisplay);
        assert(m_pEGLDisplay && "eglGetDisplay()");

        EGLint major, minor;
        if (!eglInitialize(m_pEGLDisplay, &major, &minor))
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
        if (!eglChooseConfig(m_pEGLDisplay, aAttr, &config, 1, &count) || count != 1)
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

        m_pEGLSurface = eglCreateWindowSurface(m_pEGLDisplay, config, m_window, aAttr);
        if (m_pEGLSurface == EGL_NO_SURFACE)
        {
            CERR("pEGLSurface == EGL_NO_SURFACE\n");
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

        m_pEGLContext =eglCreateContext(m_pEGLDisplay, config, EGL_NO_CONTEXT, aAttr);
        if (m_pEGLContext == EGL_NO_CONTEXT)
        {
            CERR("context == EGL_NO_CONTEXT\n");
            exit(1);
        }
    }

    XStoreName(m_pDisplay, m_window, m_sName.data());

    XSelectInput(
        m_pDisplay,
        m_window,
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

    m_bRunning = true;
}

void
Win::destroy()
{
    if (m_pEGLDisplay) eglTerminate(m_pEGLDisplay);
    XDestroyWindow(m_pDisplay, m_window);
    if (m_pDisplay) XCloseDisplay(m_pDisplay);
}

void
Win::enableRelativeMode()
{
}

void
Win::disableRelativeMode()
{
}

void
Win::hideCursor()
{
}

void
Win::setCursorImage([[maybe_unused]] String cursorType)
{
}

void
Win::setFullscreen()
{
    if (m_bFullscreen) return;

    m_bFullscreen = true;
    Atom atomWmState = XInternAtom(m_pDisplay, "_NET_WM_STATE", true);
    Atom atomWmFullscreen = XInternAtom(m_pDisplay, "_NET_WM_STATE_FULLSCREEN", true);

    XChangeProperty(m_pDisplay, m_window, atomWmState, XA_ATOM, 32, PropModeReplace, (unsigned char*)&atomWmFullscreen, 1);
}

void
Win::unsetFullscreen()
{
    m_bFullscreen = false;
}

void
Win::togglePointerRelativeMode()
{
}

void
Win::toggleFullscreen()
{
    if (m_bFullscreen) unsetFullscreen();
    else setFullscreen();

    LOG_NOTIFY("bFullscreen: {}\n", m_bFullscreen);
}

void
Win::bindGlContext()
{
    EGLD ( eglMakeCurrent(m_pEGLDisplay, m_pEGLSurface, m_pEGLSurface, m_pEGLContext) );
}

void
Win::unbindGlContext()
{
    EGLD( eglMakeCurrent(m_pEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}

void
Win::setSwapInterval(int interval)
{
    m_swapInterval = interval;
    EGLD( eglSwapInterval(m_pEGLDisplay, interval) );
}

void
Win::toggleVSync()
{
    if (m_swapInterval == 1) m_swapInterval = -1;
    else m_swapInterval = 1;

    EGLD( eglSwapInterval(m_pEGLDisplay, m_swapInterval) );
    LOG_OK("swapInterval: {}\n", m_swapInterval);
}

void
Win::swapBuffers()
{
    EGLD( eglSwapBuffers(m_pEGLDisplay, m_pEGLSurface) );
}

void
Win::procEvents()
{
    input::procEvents(this);
}

void
Win::showWindow()
{
    XMapRaised(m_pDisplay, m_window);
}

} /* namespace x11 */
} /* namespace platform */
