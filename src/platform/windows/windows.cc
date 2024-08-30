#include "windows.hh"
#include "input.hh"
#include "logs.hh"
#include "../../gl/gl.hh"
#include "wglext.h" /* https://www.khronos.org/registry/OpenGL/api/GL/wglext.h */

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

    for (u32 i = 0; i < sExt.size(); )
    {
        u32 start = i;
        u32 end = i;

        auto skipSpace = [&]() -> void {
            while (end < sExt.size() && sExt[end] != ' ')
                end++;
        };

        skipSpace();

        String sWord(&sExt[start], end - start);
        LOG_OK("'%.*s'\n", sWord.size(), sWord.data());

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

Window::Window(Allocator* p, String sName, HINSTANCE hInstance)
{
    _pAlloc = p;
    _svName = sName;
    _hInstance = hInstance;
    init();
}

Window::~Window()
{
}

void
Window::init()
{
    getWglFunctions();

    _windowClass = {};
    _windowClass.cbSize = sizeof(_windowClass);
    _windowClass.lpfnWndProc = input::windowProc;
    _windowClass.hInstance = _hInstance;
    _windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    _windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    _windowClass.lpszClassName = L"opengl_window_class";

    ATOM atom = RegisterClassExW(&_windowClass);
    if (!atom) LOG_FATAL("RegisterClassExW failed\n");

    _wWidth = 1280;
    _wHeight = 960;
    DWORD exstyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    RECT rect = { 0, 0, 1280, 960 };
    AdjustWindowRectEx(&rect, style, false, exstyle);
    _wWidth = rect.right - rect.left;
    _wHeight = rect.bottom - rect.top;

    _hWindow = CreateWindowExW(exstyle,
                             _windowClass.lpszClassName,
                             L"OpenGL Window",
                             style,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             _wWidth,
                             _wHeight,
                             nullptr,
                             nullptr,
                             _windowClass.hInstance,
                             this);
    if (!_hWindow) LOG_FATAL("CreateWindowExW failed\n");

    _hDeviceContext = GetDC(_hWindow);
    if (!_hDeviceContext) LOG_FATAL("GetDC failed\n");

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
    if (!wglChoosePixelFormatARB(_hDeviceContext, attrib, nullptr, 1, &format, &formats) || formats == 0)
        LOG_FATAL("OpenGL does not support required pixel format!");

    PIXELFORMATDESCRIPTOR desc {};
    desc.nSize = sizeof(desc);
    int ok = DescribePixelFormat(_hDeviceContext, format, sizeof(desc), &desc);
    if (!ok) LOG_FATAL("DescribePixelFormat failed\n");

    if (!SetPixelFormat(_hDeviceContext, format, &desc)) LOG_FATAL("Cannot set OpenGL selected pixel format!");

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

    _hGlContext = wglCreateContextAttribsARB(_hDeviceContext, nullptr, attribContext);
    if (!_hGlContext) LOG_FATAL("Cannot create OpenGL context! OpenGL version 4.5 is not supported");

    bool okContext = wglMakeCurrent(_hDeviceContext, _hGlContext);
    if (!okContext) LOG_FATAL("wglMakeCurrent failed\n");

    if (!gladLoadGL()) LOG_FATAL("gladLoadGL failed\n");

    unbindGlContext();
}

void
Window::disableRelativeMode()
{
    _bRelativeMode = false;
    input::registerRawMouseDevice(this, false);
}

void
Window::enableRelativeMode()
{
    _bRelativeMode = true;
    input::registerRawMouseDevice(this, true);
}

void
Window::togglePointerRelativeMode()
{
    _bRelativeMode = !_bRelativeMode;
    _bRelativeMode ? enableRelativeMode() : disableRelativeMode();
    LOG_OK("relative mode: %d\n", _bRelativeMode);
}

void
Window::toggleFullscreen()
{
    _bFullscreen = !_bFullscreen;
    _bFullscreen ? setFullscreen() : unsetFullscreen();
    LOG_OK("fullscreen: %d\n", _bRelativeMode);
}

void 
Window::setCursorImage(String cursorType)
{
    /* TODO: */
}

void 
Window::setFullscreen() 
{
    input::enterFullscreen(_hWindow,
                           GetDeviceCaps(_hDeviceContext, 0),
                           GetDeviceCaps(_hDeviceContext, 0),
                           GetDeviceCaps(_hDeviceContext, 0),
                           GetDeviceCaps(_hDeviceContext, 0));
}

void
Window::unsetFullscreen()
{
    input::exitFullscreen(_hWindow, 0, 0, 800, 600, 0, 0);
}

void 
Window::bindGlContext()
{
    wglMakeCurrent(_hDeviceContext, _hGlContext);
}

void 
Window::unbindGlContext()
{
    wglMakeCurrent(nullptr, nullptr);
}

void
Window::setSwapInterval(int interval)
{
    _swapInterval = interval;
    wglSwapIntervalEXT(interval);
}

void 
Window::toggleVSync()
{
    _swapInterval = !_swapInterval;
    wglSwapIntervalEXT(_swapInterval);
    LOG_OK("swapInterval: %d\n", _swapInterval);
}

void
Window::swapBuffers()
{
    if (!SwapBuffers(_hDeviceContext)) LOG_WARN("SwapBuffers(dc): failed\n");
}

void 
Window::procEvents()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_QUIT:
                _bRunning = false;
                break;

            default:
                break;
        };

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void
Window::showWindow()
{
    ShowWindow(_hWindow, SW_SHOWDEFAULT);
}

} /* namespace win32 */
