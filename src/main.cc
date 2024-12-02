#ifdef _WIN32
    /* win32: must be ontop */
    #include "platform/win32/Mixer.hh"
    #include "platform/win32/Win32Window.hh"
#endif

#include "adt/Arena.hh"
#include "app.hh"
#include "frame.hh"

using namespace adt;

static Arena s_arena(SIZE_1M);

#ifdef __linux__
    #include "platform/wayland/Client.hh"
    #include "platform/pipewire/Mixer.hh"

#ifdef X11_LIB
    #include "platform/x11/X11Window.hh"
#endif

int
main(int argc, char** argv)
{
    app::g_argc = argc, app::g_argv = argv;

    auto tpool = ThreadPool(&s_arena.super, utils::max(getNCores() - 2, 2));
    ThreadPoolStart(&tpool);
    app::g_pThreadPool = &tpool;

    bool bWayland = false;
    bool bX11 = false;

#ifdef X11_LIB
    if (argc > 1 && String(argv[1]) == "--x11") bX11 = true;
    platform::x11::Window x11("Breakout");
#endif

    platform::wayland::Client wlClient("Breakout");

    const char* pWaylandDisplayEnv = getenv("WAYLAND_DISPLAY");
    if (pWaylandDisplayEnv && !bX11)
    {
        print::err("wayland display: '{}'\n", pWaylandDisplayEnv);
        bWayland = true;
    }

    if (bWayland)
    {
        ::WindowStart(&wlClient);
        app::g_pWindow = &wlClient.super;
    }
    else
    {
#ifdef X11_LIB
        ::WindowStart(&x11);
        app::g_pWindow = &x11.super;
#else
        print::err("Failed to create window (no display server?)\n");
#endif
    }

    platform::pipewire::Mixer mixer(&s_arena.super);
    audio::MixerStart(&mixer);
    app::g_pMixer = &mixer.super;

    frame::run();

    ThreadPoolWait(&tpool);
    ThreadPoolDestroy(&tpool);

    /* mixer is destroyed after frame::mainLoop() */
    if (bWayland) ::WindowDestroy(&wlClient);

#ifdef X11_LIB
    if (bX11) ::WindowDestroy(&x11);
#endif

#ifndef NDEBUG
    ArenaFreeAll(&s_arena);
#endif
}

#elif _WIN32

int WINAPI
WinMain(
    HINSTANCE instance,
    [[maybe_unused]] HINSTANCE previnstance,
    [[maybe_unused]] LPSTR cmdline,
    [[maybe_unused]] int cmdshow)
{
    auto tpool = ThreadPool(&s_arena.super, utils::max(getNCores() - 2, 2));
    ThreadPoolStart(&tpool);
    app::g_pThreadPool = &tpool;

    platform::win32::Mixer mixer(&s_arena.super);
    platform::win32::Win32Window app("Breakout", instance);

    app::g_pMixer = &mixer.super;
    app::g_pWindow = &app.super;

    frame::run();

    WindowDestroy(&app);

#ifndef NDEBUG
    ArenaFreeAll(&s_arena);
#endif
}

    #ifndef NDEBUG
int
main(int argc, char** argv)
{
    app::g_argc = argc, app::g_argv = argv;

    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
    #endif

#endif
