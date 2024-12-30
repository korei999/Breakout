#include "Win32Window.hh"

#include "input.hh"
#include "adt/logs.hh"
#include "gl/gl.hh"
#include "wglext.h" /* https://www.khronos.org/registry/OpenGL/api/GL/wglext.h */

namespace platform
{
namespace win32
{

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB {};
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB {};
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT {};

static void
getWglFunctions(void)
{
    /* to get WGL functions we need valid GL context, so create dummy window for dummy GL contetx */
    HWND dummy = CreateWindowExW(
        0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, nullptr, nullptr);
    if (!dummy) LOG_FATAL("CreateWindowExW failed\n");

    HDC dc = GetDC(dummy);
    if (!dc) LOG_FATAL("GetDC failed\n");

    PIXELFORMATDESCRIPTOR desc {};
    desc.nSize = sizeof(desc);
    desc.nVersion = 1;
    desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    desc.iPixelType = PFD_TYPE_RGBA;
    desc.cColorBits = 24;

    int format = ChoosePixelFormat(dc, &desc);
    if (!format) LOG_FATAL("Cannot choose OpenGL pixel format for dummy window!");

    int ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    if (!ok) LOG_FATAL("DescribePixelFormat failed\n");

    // reason to create dummy window is that SetPixelFormat can be called only once for the window
    if (!SetPixelFormat(dc, format, &desc)) LOG_FATAL("Cannot set OpenGL pixel format for dummy window!");

    HGLRC rc = wglCreateContext(dc);
    if (!rc) LOG_FATAL("wglCreateContext failed\n");

    ok = wglMakeCurrent(dc, rc);
    if (!ok) LOG_FATAL("wglMakeCurrent failed\n");

    // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt
    auto wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (!wglGetExtensionsStringARB) LOG_FATAL("OpenGL does not support WGL_ARB_extensions_string extension!");

    const char* ext = wglGetExtensionsStringARB(dc);
    if (!ext) LOG_FATAL("wglGetExtensionsStringARB failed\n");

    String sExt(ext);

    for (u32 i = 0; i < sExt.getSize(); )
    {
        u32 start = i;
        u32 end = i;

        auto skipSpace = [&]() -> void {
            while (end < sExt.getSize() && sExt[end] != ' ')
                end++;
        };

        skipSpace();

        String sWord(&sExt[start], end - start);
        LOG_OK("'{}'\n", sWord);

        if (sWord == "WGL_ARB_pixel_format")
            wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        else if (sWord == "WGL_ARB_create_context")
            wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        else if (sWord == "WGL_EXT_swap_control")
            wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

        i = end + 1;
    }

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB || !wglSwapIntervalEXT)
        LOG_FATAL("OpenGL does not support required WGL extensions context!");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rc);
    ReleaseDC(dummy, dc);
    DestroyWindow(dummy);
}

void
Win32Window::start()
{
    getWglFunctions();

    this->windowClass = {};
    this->windowClass.cbSize = sizeof(this->windowClass);
    this->windowClass.lpfnWndProc = input::windowProc;
    this->windowClass.hInstance = this->_hInstance;
    this->windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    this->windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    this->windowClass.lpszClassName = L"opengl_window_class";

    ATOM atom = RegisterClassExW(&this->windowClass);
    if (!atom) LOG_FATAL("RegisterClassExW failed\n");

    this->m_wWidth = 1280;
    this->m_wHeight = 960;
    DWORD exstyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    RECT rect = { 0, 0, 1280, 960 };
    AdjustWindowRectEx(&rect, style, false, exstyle);
    this->m_wWidth = rect.right - rect.left;
    this->m_wHeight = rect.bottom - rect.top;

    this->hWindow = CreateWindowExW(exstyle,
        this->windowClass.lpszClassName,
        L"Breakout",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        this->m_wWidth,
        this->m_wHeight,
        nullptr,
        nullptr,
        this->windowClass.hInstance,
        this
    );

    if (!this->hWindow) LOG_FATAL("CreateWindowExW failed\n");

    this->hDeviceContext = GetDC(this->hWindow);
    if (!this->hDeviceContext) LOG_FATAL("GetDC failed\n");

    /* FIXME: find better way to toggle this on startup */
    input::registerRawMouseDevice(this, true);

    int attrib[] {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     24,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,

        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,

        WGL_SAMPLE_BUFFERS_ARB, 1,
        WGL_SAMPLES_ARB,        4,
        0,
    };

    int format;
    UINT formats;
    if (!wglChoosePixelFormatARB(this->hDeviceContext, attrib, nullptr, 1, &format, &formats) || formats == 0)
        LOG_FATAL("OpenGL does not support required pixel format!");

    PIXELFORMATDESCRIPTOR desc {};
    desc.nSize = sizeof(desc);
    int ok = DescribePixelFormat(this->hDeviceContext, format, sizeof(desc), &desc);
    if (!ok) LOG_FATAL("DescribePixelFormat failed\n");

    if (!SetPixelFormat(this->hDeviceContext, format, &desc)) LOG_FATAL("Cannot set OpenGL selected pixel format!");

    int attribContext[] {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifndef NDEBUG
        // ask for debug context for non "Release" builds
        // this is so we can enable debug callback
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0,
    };

    this->hGlContext = wglCreateContextAttribsARB(this->hDeviceContext, nullptr, attribContext);
    if (!this->hGlContext) LOG_FATAL("Cannot create OpenGL context! OpenGL version 4.5 is not supported");

    bool okContext = wglMakeCurrent(this->hDeviceContext, this->hGlContext);
    if (!okContext) LOG_FATAL("wglMakeCurrent failed\n");

    if (!gladLoadGL()) LOG_FATAL("gladLoadGL failed\n");

    unbindGlContext();

    this->m_bPointerRelativeMode = false;
    this->m_bPaused = false;
}

void
Win32Window::destroy()
{
    DestroyWindow(hWindow);
}

void
Win32Window::disableRelativeMode()
{
    m_bPointerRelativeMode = false;
    input::registerRawMouseDevice(this, false);
}

void
Win32Window::enableRelativeMode()
{
    m_bPointerRelativeMode = true;
    input::registerRawMouseDevice(this, true);
}

void
Win32Window::togglePointerRelativeMode()
{
    this->m_bPointerRelativeMode = !this->m_bPointerRelativeMode;
    this->m_bPointerRelativeMode ? enableRelativeMode() : disableRelativeMode();
    LOG_OK("relative mode: {}\n", this->m_bPointerRelativeMode);
}

void
Win32Window::toggleFullscreen()
{
    this->m_bFullscreen = !this->m_bFullscreen;
    this->m_bFullscreen ? setFullscreen() : unsetFullscreen();
    LOG_OK("fullscreen: {}\n", this->m_bPointerRelativeMode);
}

void 
Win32Window::hideCursor()
{
    /* TODO: */
}

void 
Win32Window::setCursorImage([[maybe_unused]] String cursorType)
{
    /* TODO: */
}

void 
Win32Window::setFullscreen() 
{
    m_bFullscreen = true;

    input::enterFullscreen(
        this->hWindow,
        GetDeviceCaps(this->hDeviceContext, 0),
        GetDeviceCaps(this->hDeviceContext, 1),
        GetDeviceCaps(this->hDeviceContext, 2),
        GetDeviceCaps(this->hDeviceContext, 3)
    );
}

void
Win32Window::unsetFullscreen()
{
    this->m_bFullscreen = false;

    input::exitFullscreen(this->hWindow, 0, 0, 800, 600, 0, 0);
}

void 
Win32Window::bindGlContext()
{
    wglMakeCurrent(hDeviceContext, hGlContext);
}

void 
Win32Window::unbindGlContext()
{
    wglMakeCurrent(nullptr, nullptr);
}

void
Win32Window::setSwapInterval(int interval)
{
    m_swapInterval = interval;
    wglSwapIntervalEXT(interval);
}

void 
Win32Window::toggleVSync()
{
    this->m_swapInterval = !this->m_swapInterval;
    wglSwapIntervalEXT(this->m_swapInterval);
    LOG_OK("swapInterval: {}\n", this->m_swapInterval);
}

void
Win32Window::swapBuffers()
{
    if (!SwapBuffers(this->hDeviceContext)) LOG_WARN("SwapBuffers(dc): failed\n");
}

void 
Win32Window::procEvents()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_QUIT:
                m_bRunning = false;
                break;

            default:
                break;
        };

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void
Win32Window::showWindow()
{
    ShowWindow(hWindow, SW_SHOWDEFAULT);
}

} /* namespace win32 */
} /* namespace platform */
