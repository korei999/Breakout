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

    FreeList alloc(SIZE_1M);
    defer( alloc.freeAll() );

    auto tpool = ThreadPool(&alloc, utils::max(getNCores() - 2, 2));
    tpool.start();
    app::g_pThreadPool = &tpool;

    app::g_pMixer = app::platformMixerAlloc(&alloc);
    app::g_pMixer->start();

    app::g_pWindow = app::platformWindowAlloc(&alloc);
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

    try
    {

    #if defined _WIN32
        return WinMain({}, {}, {}, SW_SHOWNORMAL);
    #else
        return startup();
    #endif
    }
    catch (IException& ex)
    {
        ex.logErrorMsg(stderr);
    }
}
#endif
