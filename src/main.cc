#ifdef _WIN32
    /* has to be included on top because of amazing win32 macros... */
    #include "platform/win32/Mixer.hh"
    #include "platform/win32/Win32Window.hh"
#endif

#include "adt/Arena.hh"
#include "app.hh"
#include "frame.hh"

using namespace adt;

Arena arena(SIZE_1M);

#ifdef __linux__
    #include "platform/wayland/Client.hh"
    #include "platform/pipewire/Mixer.hh"

int
main(int argc, char** argv)
{
    app::g_argc = argc, app::g_argv = argv;

    platform::pipewire::Mixer mixer(&arena.base);
    platform::wayland::Client window("Breakout");

    WindowInit(&window);
    audio::MixerInit(&mixer);

    app::g_pWindow = &window.base;
    app::g_pMixer = &mixer.base;

    frame::run();

#ifndef NDEBUG
    audio::MixerDestroy(&mixer);
    WindowDestroy(&window);
    ArenaFreeAll(&arena);
#endif
}

#elif _WIN32

int WINAPI
WinMain([[maybe_unused]] HINSTANCE instance,
        [[maybe_unused]] HINSTANCE previnstance,
        [[maybe_unused]] LPSTR cmdline,
        [[maybe_unused]] int cmdshow)
{
    platform::win32::Mixer mixer(&arena.base);
    platform::win32::Win32Window app("Breakout", instance);

    app::g_pMixer = &mixer.base;
    app::g_pWindow = &app.base;

    frame::run();

#ifndef NDEBUG
    platform::win32::MixerDestroy(&mixer);
    platform::win32::Win32Destroy(&app);
    ArenaFreeAll(&arena);
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
