#ifdef _WIN32
    /* win32: must be ontop */
    #include "platform/win32/Mixer.hh"
    #include "platform/win32/Win32Window.hh"
#endif

#include "app.hh"

#include "adt/logs.hh" /* IWYU pragma: keep */

#ifdef __linux__
    #include "platform/pipewire/Mixer.hh"
    #include "platform/wayland/Client.hh"

    #ifdef X11_LIB
        #include "platform/x11/X11Window.hh"
    #endif
#endif

namespace app
{

adt::ThreadPool* g_pThreadPool;

int g_argc = 0;
char** g_argv = nullptr;

audio::IMixer* g_pMixer;
IWindow* g_pWindow;

audio::IMixer*
platformMixerAlloc(IAllocator* pAlloc)
{
#ifdef __linux__
    {
        namespace pw = platform::pipewire;

        auto* pMixer = (pw::Mixer*)pAlloc->zalloc(1, sizeof(pw::Mixer));
        new(pMixer) pw::Mixer(pAlloc);

        return pMixer;
    }
#elif defined _WIN32
    {
        namespace p32 = platform::win32;

        auto* pMixer = (p32::Mixer*)pAlloc->zalloc(1, sizeof(p32::Mixer));
        new(pMixer) p32::Mixer(pAlloc);

        return pMixer;
    }
#else
    {
        auto* pMixer = (audio::DummyMixer*)pAlloc->zalloc(1, sizeof(audio::DummyMixer));
        new(pMixer) audio::DummyMixer();
        LOG_BAD("Cannot create audio mixer\n");
        return pMixer;
    }
#endif
}

#ifdef _WIN32
static IWindow*
win32WindowAlloc(IAllocator* pAlloc)
{
    namespace w32 = platform::win32;

    HMODULE hInstance = GetModuleHandle(nullptr);

    auto* pWindow = (IWindow*)pAlloc->zalloc(1, sizeof(w32::Win32Window));
    new(pWindow) w32::Win32Window("Breakout", hInstance);

    return pWindow;
}
#endif

IWindow*
platformWindowAlloc(IAllocator* pAlloc)
{
#ifdef __linux__
    #ifdef X11_LIB
    namespace x = platform::x11;
    #endif
    namespace wl = platform::wayland;

    bool bWayland = false;
    bool bX11 = false;

    #ifdef X11_LIB
    if (g_argc > 1 && String(g_argv[1]) == "--x11")
        bX11 = true;
    #endif

    IWindow* pWindow = nullptr;

    const char* pWaylandDisplayEnv = getenv("WAYLAND_DISPLAY");
    if (pWaylandDisplayEnv && !bX11)
    {
        print::err("wayland display: '{}'\n", pWaylandDisplayEnv);
        bWayland = true;
    }

    if (bWayland)
    {
        pWindow = (IWindow*)pAlloc->zalloc(1, sizeof(wl::Client));
        new(pWindow) wl::Client("Breakout");
    }
    else
    {
    #ifdef X11_LIB
        pWindow = (IWindow*)pAlloc->zalloc(1, sizeof(x::Win));
        new(pWindow) x::Win("Breakout");
    #else
        print::err("Can't create graphical window\n");
    #endif
    }

    return pWindow;
#elif defined _WIN32
    return win32WindowAlloc(pAlloc);
#else
    #error "Platform window"
#endif
}

} /* namespace app */
