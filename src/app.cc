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
    #include "platform/wayland/ClientSW.hh"

    #ifdef X11_LIB
        #include "platform/x11/X11Window.hh"
    #endif
#endif

namespace app
{

WINDOW_TYPE g_eWindowType {};

adt::ThreadPool* g_pThreadPool {};

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

        return pAlloc->alloc<pw::Mixer>(pAlloc);
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
        return pAlloc->alloc<audio::DummyMixer>();
    }
#endif
}

IWindow*
platformWindowAlloc(IAllocator* pAlloc)
{
    IWindow* pWindow = nullptr;

    switch (g_eWindowType)
    {
        default: break;

#ifdef __linux__
        case WINDOW_TYPE::WAYLAND_SW:
        return pAlloc->alloc<platform::wayland::ClientSW>("Breakout");

        case WINDOW_TYPE::WAYLAND_GL:
        return pAlloc->alloc<platform::wayland::Client>("Breakout");

        case WINDOW_TYPE::X11_GL:
        throw RuntimeException("not implemented");
#endif

#ifdef _WIN32
        case WINDOW_TYPE::WIN11_GL:
        {
            HMODULE hInstance = GetModuleHandle(nullptr);
            return pAlloc->alloc<platform::win32::Win32Window>("Breakout", hInstance);
        }
#endif
    }

    throw RuntimeException("platformWindowAlloc() failed");
}

} /* namespace app */
