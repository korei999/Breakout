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

    app::g_pMixer = app::platformMixerAlloc(&s_arena.super);
    audio::MixerStart(app::g_pMixer);

    app::g_pWindow = app::platformWindowAlloc(&s_arena.super);
    WindowStart(app::g_pWindow);

    frame::run();

    ThreadPoolWait(&tpool);
    ThreadPoolDestroy(&tpool);

    /* mixer is destroyed after frame::mainLoop() */
    WindowDestroy(app::g_pWindow);

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
    platform::win32::Win32Window window("Breakout", instance);

    MixerStart(&mixer);
    WindowStart(&window);

    app::g_pMixer = &mixer.super;
    app::g_pWindow = &window.super;

    frame::run();

    WindowDestroy(&window);

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
