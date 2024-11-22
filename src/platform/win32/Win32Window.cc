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

Win32Window::Win32Window(String sName, HINSTANCE hInstance)
{
    static WindowInterface vTable {
        .init = (decltype(WindowInterface::init))Win32Init,
        .disableRelativeMode = (decltype(WindowInterface::disableRelativeMode))Win32DisableRelativeMode,
        .enableRelativeMode = (decltype(WindowInterface::enableRelativeMode))Win32EnableRelativeMode,
        .togglePointerRelativeMode = (decltype(WindowInterface::togglePointerRelativeMode))Win32TogglePointerRelativeMode,
        .toggleFullscreen = (decltype(WindowInterface::toggleFullscreen))Win32ToggleFullscreen,
        .hideCursor = (decltype(WindowInterface::hideCursor))Win32HideCursor,
        .setCursorImage = (decltype(WindowInterface::setCursorImage))Win32SetCursorImage,
        .setFullscreen = (decltype(WindowInterface::setFullscreen))Win32SetFullscreen,
        .unsetFullscreen = (decltype(WindowInterface::unsetFullscreen))Win32UnsetFullscreen,
        .bindGlContext = (decltype(WindowInterface::bindGlContext))Win32BindGlContext,
        .unbindGlContext = (decltype(WindowInterface::unbindGlContext))Win32UnbindGlContext,
        .setSwapInterval = (decltype(WindowInterface::setSwapInterval))Win32SetSwapInterval,
        .toggleVSync = (decltype(WindowInterface::toggleVSync))Win32ToggleVSync,
        .swapBuffers = (decltype(WindowInterface::swapBuffers))Win32SwapBuffers,
        .procEvents = (decltype(WindowInterface::procEvents))Win32ProcEvents,
        .showWindow = (decltype(WindowInterface::showWindow))Win32ShowWindow,
        .destroy = (decltype(WindowInterface::destroy))Win32Destroy,
    };

    this->super.pVTable = {&vTable};

    this->super.sName = sName;
    this->hInstance = hInstance;
    Win32Init(this);
}


void
Win32Destroy([[maybe_unused]] Win32Window* s)
{
}

void
Win32Init(Win32Window* s)
{
    getWglFunctions();

    s->windowClass = {};
    s->windowClass.cbSize = sizeof(s->windowClass);
    s->windowClass.lpfnWndProc = input::windowProc;
    s->windowClass.hInstance = s->hInstance;
    s->windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    s->windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    s->windowClass.lpszClassName = L"opengl_window_class";

    ATOM atom = RegisterClassExW(&s->windowClass);
    if (!atom) LOG_FATAL("RegisterClassExW failed\n");

    s->super.wWidth = 1280;
    s->super.wHeight = 960;
    DWORD exstyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    RECT rect = { 0, 0, 1280, 960 };
    AdjustWindowRectEx(&rect, style, false, exstyle);
    s->super.wWidth = rect.right - rect.left;
    s->super.wHeight = rect.bottom - rect.top;

    s->hWindow = CreateWindowExW(exstyle,
        s->windowClass.lpszClassName,
        L"Breakout",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        s->super.wWidth,
        s->super.wHeight,
        nullptr,
        nullptr,
        s->windowClass.hInstance,
        s
    );

    if (!s->hWindow) LOG_FATAL("CreateWindowExW failed\n");

    s->hDeviceContext = GetDC(s->hWindow);
    if (!s->hDeviceContext) LOG_FATAL("GetDC failed\n");

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
    if (!wglChoosePixelFormatARB(s->hDeviceContext, attrib, nullptr, 1, &format, &formats) || formats == 0)
        LOG_FATAL("OpenGL does not support required pixel format!");

    PIXELFORMATDESCRIPTOR desc {};
    desc.nSize = sizeof(desc);
    int ok = DescribePixelFormat(s->hDeviceContext, format, sizeof(desc), &desc);
    if (!ok) LOG_FATAL("DescribePixelFormat failed\n");

    if (!SetPixelFormat(s->hDeviceContext, format, &desc)) LOG_FATAL("Cannot set OpenGL selected pixel format!");

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

    s->hGlContext = wglCreateContextAttribsARB(s->hDeviceContext, nullptr, attribContext);
    if (!s->hGlContext) LOG_FATAL("Cannot create OpenGL context! OpenGL version 4.5 is not supported");

    bool okContext = wglMakeCurrent(s->hDeviceContext, s->hGlContext);
    if (!okContext) LOG_FATAL("wglMakeCurrent failed\n");

    if (!gladLoadGL()) LOG_FATAL("gladLoadGL failed\n");

    Win32UnbindGlContext(s);

    s->super.bPointerRelativeMode = false;
    s->super.bPaused = false;
}

void
Win32DisableRelativeMode(Win32Window* s)
{
    s->super.bPointerRelativeMode = false;
    input::registerRawMouseDevice(s, false);
}

void
Win32EnableRelativeMode(Win32Window* s)
{
    s->super.bPointerRelativeMode = true;
    input::registerRawMouseDevice(s, true);
}

void
Win32TogglePointerRelativeMode(Win32Window* s)
{
    s->super.bPointerRelativeMode = !s->super.bPointerRelativeMode;
    s->super.bPointerRelativeMode ? Win32EnableRelativeMode(s) : Win32DisableRelativeMode(s);
    LOG_OK("relative mode: {}\n", s->super.bPointerRelativeMode);
}

void
Win32ToggleFullscreen(Win32Window* s)
{
    s->super.bFullscreen = !s->super.bFullscreen;
    s->super.bFullscreen ? Win32SetFullscreen(s) : Win32UnsetFullscreen(s);
    LOG_OK("fullscreen: {}\n", s->super.bPointerRelativeMode);
}

void 
Win32HideCursor([[maybe_unused]] Win32Window* s)
{
    /* TODO: */
}

void 
Win32SetCursorImage([[maybe_unused]] Win32Window* s, [[maybe_unused]] String cursorType)
{
    /* TODO: */
}

void 
Win32SetFullscreen(Win32Window* s) 
{
    s->super.bFullscreen = true;

    input::enterFullscreen(
        s->hWindow,
        GetDeviceCaps(s->hDeviceContext, 0),
        GetDeviceCaps(s->hDeviceContext, 1),
        GetDeviceCaps(s->hDeviceContext, 2),
        GetDeviceCaps(s->hDeviceContext, 3)
    );
}

void
Win32UnsetFullscreen(Win32Window* s)
{
    s->super.bFullscreen = false;

    input::exitFullscreen(s->hWindow, 0, 0, 800, 600, 0, 0);
}

void 
Win32BindGlContext(Win32Window* s)
{
    wglMakeCurrent(s->hDeviceContext, s->hGlContext);
}

void 
Win32UnbindGlContext([[maybe_unused]] Win32Window* s)
{
    wglMakeCurrent(nullptr, nullptr);
}

void
Win32SetSwapInterval(Win32Window* s, int interval)
{
    s->super.swapInterval = interval;
    wglSwapIntervalEXT(interval);
}

void 
Win32ToggleVSync(Win32Window* s)
{
    s->super.swapInterval = !s->super.swapInterval;
    wglSwapIntervalEXT(s->super.swapInterval);
    LOG_OK("swapInterval: {}\n", s->super.swapInterval);
}

void
Win32SwapBuffers(Win32Window* s)
{
    if (!SwapBuffers(s->hDeviceContext)) LOG_WARN("SwapBuffers(dc): failed\n");
}

void 
Win32ProcEvents(Win32Window* s)
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_QUIT:
                s->super.bRunning = false;
                break;

            default:
                break;
        };

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void
Win32ShowWindow(Win32Window* s)
{
    ShowWindow(s->hWindow, SW_SHOWDEFAULT);
}

} /* namespace win32 */
} /* namespace platform */
