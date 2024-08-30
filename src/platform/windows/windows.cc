#include "windows.hh"
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

    COUT("'%s'\n\n", ext);

    String sExt(ext);

    for (u32 i = 0; i < sExt.size; )
    {
        u32 start = i;
        u32 end = i;

        auto skipSpace = [&]() -> void {
            while (end < sExt.size && sExt[end] != ' ')
                end++;
        };

        skipSpace();

        String sWord(&sExt[start], end - start);
        LOG_OK("'%.*s'\n", sWord.size, sWord.pData);

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

Window::Window(String sName, HINSTANCE hInstance)
{
    static AppInterface vTable {
        .init = (decltype(AppInterface::init))WindowInit,
        .disableRelativeMode = (decltype(AppInterface::disableRelativeMode))WindowDisableRelativeMode,
        .enableRelativeMode = (decltype(AppInterface::enableRelativeMode))WindowEnableRelativeMode,
        .togglePointerRelativeMode = (decltype(AppInterface::togglePointerRelativeMode))WindowTogglePointerRelativeMode,
        .toggleFullscreen = (decltype(AppInterface::toggleFullscreen))WindowToggleFullscreen,
        .setCursorImage = (decltype(AppInterface::setCursorImage))WindowSetCursorImage,
        .setFullscreen = (decltype(AppInterface::setFullscreen))WindowSetFullscreen,
        .unsetFullscreen = (decltype(AppInterface::unsetFullscreen))WindowUnsetFullscreen,
        .bindGlContext = (decltype(AppInterface::bindGlContext))WindowBindGlContext,
        .unbindGlContext = (decltype(AppInterface::unbindGlContext))WindowUnbindGlContext,
        .setSwapInterval = (decltype(AppInterface::setSwapInterval))WindowSetSwapInterval,
        .toggleVSync = (decltype(AppInterface::toggleVSync))WindowToggleVSync,
        .swapBuffers = (decltype(AppInterface::swapBuffers))WindowSwapBuffers,
        .procEvents = (decltype(AppInterface::procEvents))WindowProcEvents,
        .showWindow = (decltype(AppInterface::showWindow))WindowShowWindow,
        .destroy = (decltype(AppInterface::destroy))WindowDestroy,
    };

    this->base.pVTable = {&vTable};

    this->base.sName = sName;
    this->_hInstance = hInstance;
    WindowInit(this);
}


void
WindowDestroy(Window* s)
{
}

void
WindowInit(Window* s)
{
    getWglFunctions();

    s->_windowClass = {};
    s->_windowClass.cbSize = sizeof(s->_windowClass);
    s->_windowClass.lpfnWndProc = input::windowProc;
    s->_windowClass.hInstance = s->_hInstance;
    s->_windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    s->_windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    s->_windowClass.lpszClassName = L"opengl_window_class";

    ATOM atom = RegisterClassExW(&s->_windowClass);
    if (!atom) LOG_FATAL("RegisterClassExW failed\n");

    s->base.wWidth = 1280;
    s->base.wHeight = 960;
    DWORD exstyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    RECT rect = { 0, 0, 1280, 960 };
    AdjustWindowRectEx(&rect, style, false, exstyle);
    s->base.wWidth = rect.right - rect.left;
    s->base.wHeight = rect.bottom - rect.top;

    s->_hWindow = CreateWindowExW(exstyle,
                             s->_windowClass.lpszClassName,
                             L"OpenGL Window",
                             style,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             s->base.wWidth,
                             s->base.wHeight,
                             nullptr,
                             nullptr,
                             s->_windowClass.hInstance,
                             s);
    if (!s->_hWindow) LOG_FATAL("CreateWindowExW failed\n");

    s->_hDeviceContext = GetDC(s->_hWindow);
    if (!s->_hDeviceContext) LOG_FATAL("GetDC failed\n");

    /* FIXME: find better way to toggle this on startup */
    input::registerRawMouseDevice(s, true);

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
    if (!wglChoosePixelFormatARB(s->_hDeviceContext, attrib, nullptr, 1, &format, &formats) || formats == 0)
        LOG_FATAL("OpenGL does not support required pixel format!");

    PIXELFORMATDESCRIPTOR desc {};
    desc.nSize = sizeof(desc);
    int ok = DescribePixelFormat(s->_hDeviceContext, format, sizeof(desc), &desc);
    if (!ok) LOG_FATAL("DescribePixelFormat failed\n");

    if (!SetPixelFormat(s->_hDeviceContext, format, &desc)) LOG_FATAL("Cannot set OpenGL selected pixel format!");

    int attribContext[] {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef DEBUG
        // ask for debug context for non "Release" builds
        // this is so we can enable debug callback
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0,
    };

    s->_hGlContext = wglCreateContextAttribsARB(s->_hDeviceContext, nullptr, attribContext);
    if (!s->_hGlContext) LOG_FATAL("Cannot create OpenGL context! OpenGL version 4.5 is not supported");

    bool okContext = wglMakeCurrent(s->_hDeviceContext, s->_hGlContext);
    if (!okContext) LOG_FATAL("wglMakeCurrent failed\n");

    if (!gladLoadGL()) LOG_FATAL("gladLoadGL failed\n");

    WindowUnbindGlContext(s);
}

void
WindowDisableRelativeMode(Window* s)
{
    s->base.bRelativeMode = false;
    input::registerRawMouseDevice(s, false);
}

void
WindowEnableRelativeMode(Window* s)
{
    s->base.bRelativeMode = true;
    input::registerRawMouseDevice(s, true);
}

void
WindowTogglePointerRelativeMode(Window* s)
{
    s->base.bRelativeMode = !s->base.bRelativeMode;
    s->base.bRelativeMode ? WindowEnableRelativeMode(s) : WindowDisableRelativeMode(s);
    LOG_OK("relative mode: %d\n", s->base.bRelativeMode);
}

void
WindowToggleFullscreen(Window* s)
{
    s->base.bFullscreen = !s->base.bFullscreen;
    s->base.bFullscreen ? WindowSetFullscreen(s) : WindowUnsetFullscreen(s);
    LOG_OK("fullscreen: %d\n", s->base.bRelativeMode);
}

void 
WindowSetCursorImage(Window* s, String cursorType)
{
    /* TODO: */
}

void 
WindowSetFullscreen(Window* s) 
{
    input::enterFullscreen(
        s->_hWindow,
        GetDeviceCaps(s->_hDeviceContext, 0),
        GetDeviceCaps(s->_hDeviceContext, 0),
        GetDeviceCaps(s->_hDeviceContext, 0),
        GetDeviceCaps(s->_hDeviceContext, 0)
    );
}

void
WindowUnsetFullscreen(Window* s)
{
    input::exitFullscreen(s->_hWindow, 0, 0, 800, 600, 0, 0);
}

void 
WindowBindGlContext(Window* s)
{
    wglMakeCurrent(s->_hDeviceContext, s->_hGlContext);
}

void 
WindowUnbindGlContext([[maybe_unused]] Window* s)
{
    wglMakeCurrent(nullptr, nullptr);
}

void
WindowSetSwapInterval(Window* s, int interval)
{
    s->base.swapInterval = interval;
    wglSwapIntervalEXT(interval);
}

void 
WindowToggleVSync(Window* s)
{
    s->base.swapInterval = !s->base.swapInterval;
    wglSwapIntervalEXT(s->base.swapInterval);
    LOG_OK("swapInterval: %d\n", s->base.swapInterval);
}

void
WindowSwapBuffers(Window* s)
{
    if (!SwapBuffers(s->_hDeviceContext)) LOG_WARN("SwapBuffers(dc): failed\n");
}

void 
WindowProcEvents(Window* s)
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_QUIT:
                s->base.bRunning = false;
                break;

            default:
                break;
        };

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void
WindowShowWindow(Window* s)
{
    ShowWindow(s->_hWindow, SW_SHOWDEFAULT);
}

} /* namespace win32 */
} /* namespace platform */
