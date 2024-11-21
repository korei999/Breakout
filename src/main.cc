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

int
main(int argc, char** argv)
{
    app::g_argc = argc, app::g_argv = argv;

    auto tpool = ThreadPool(&s_arena.super, utils::max(getNCores() - 2, 2));
    ThreadPoolStart(&tpool);
    app::g_pThreadPool = &tpool;

    platform::pipewire::Mixer mixer(&s_arena.super);
    platform::wayland::Client window("Breakout");

    WindowInit(&window);
    audio::MixerInit(&mixer);

    app::g_pWindow = &window.super;
    app::g_pMixer = &mixer.super;

    frame::run();

    ThreadPoolWait(&tpool);
    ThreadPoolDestroy(&tpool);

    /* mixer is destroyed after frame::mainLoop() */
    WindowDestroy(&window);

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

    app::g_pMixer = &mixer.base;
    app::g_pWindow = &app.base;

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
