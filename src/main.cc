#include "adt/Arena.hh"
#include "app.hh"
#include "frame.hh"

using namespace adt;

static int startup();

#if defined _WIN32
int WINAPI
WinMain(
    [[maybe_unused]] HINSTANCE instance,
    [[maybe_unused]] HINSTANCE previnstance,
    [[maybe_unused]] LPSTR cmdline,
    [[maybe_unused]] int cmdshow)
{
    return startup();
}
#endif

static int
startup()
{
    Arena arena(SIZE_1M);
    defer( freeAll(&arena) );

    auto tpool = ThreadPool(&arena.super, utils::max(getNCores() - 2, 2));
    ThreadPoolStart(&tpool);
    app::g_pThreadPool = &tpool;

    app::g_pMixer = app::platformMixerAlloc(&arena.super);
    audio::MixerStart(app::g_pMixer);

    app::g_pWindow = app::platformWindowAlloc(&arena.super);
    WindowStart(app::g_pWindow);

    frame::run();

    ThreadPoolWait(&tpool);
    ThreadPoolDestroy(&tpool);

    /* mixer is destroyed after frame::mainLoop() */
    WindowDestroy(app::g_pWindow);

    return 0;
}

#if (defined _WIN32 && !defined NDEBUG) || defined __linux__
int
main(int argc, char** argv)
{
    app::g_argc = argc, app::g_argv = argv;

    #if defined _WIN32
    return WinMain({}, {}, {}, SW_SHOWNORMAL);
    #else
    return startup();
    #endif
}
#endif
