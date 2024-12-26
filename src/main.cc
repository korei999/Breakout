#include "adt/FreeList.hh"
#include "app.hh"
#include "frame.hh"

#include <clocale>

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
    setlocale(LC_ALL, "");

    FreeList arena(SIZE_1M);
    defer( arena.freeAll() );

    auto tpool = ThreadPool(&arena, utils::max(getNCores() - 2, 2));
    tpool.start();
    app::g_pThreadPool = &tpool;

    app::g_pMixer = app::platformMixerAlloc(&arena);
    app::g_pMixer->start();

    app::g_pWindow = app::platformWindowAlloc(&arena);
    app::g_pWindow->start();

    frame::run();

    tpool.wait();
    tpool.destroy();

    /* mixer is destroyed after frame::mainLoop() */
    app::g_pWindow->destroy();

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
