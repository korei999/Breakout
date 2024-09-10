#include "adt/Arena.hh"
#include "app.hh"
#include "frame.hh"

using namespace adt;

Arena alMixer(SIZE_1M);

#ifdef __linux__
    #include "platform/wayland/Client.hh"
    #include "platform/pipewire/Mixer.hh"

int
main(int argc, char* argv[])
{
    platform::pipewire::Mixer mixer(&alMixer.base);
    platform::wayland::Client app("Breakout");

    WindowInit(&app);
    audio::MixerInit(&mixer, argc, argv);

    app::g_pApp = &app.base;
    app::g_pMixer = &mixer.base;

    frame::run();

    audio::MixerDestroy(&mixer);
    WindowDestroy(&app);

    ArenaFreeAll(&alMixer);
}

#elif _WIN32
    #include "platform/win32/Mixer.hh"
    #include "platform/win32/Window.hh"

int WINAPI
WinMain([[maybe_unused]] HINSTANCE instance,
        [[maybe_unused]] HINSTANCE previnstance,
        [[maybe_unused]] LPSTR cmdline,
        [[maybe_unused]] int cmdshow)
{
    platform::win32::Mixer mixer(&alMixer.base);
    platform::win32::MixerInit(&mixer, {}, {});
    platform::win32::Window app("Breakout", instance);

    frame::g_pMixer = &mixer.base;
    frame::g_pApp = &app.base;

    frame::run();

    platform::win32::MixerDestroy(&mixer);
    platform::win32::WindowDestroy(&app);

    ArenaFreeAll(&alMixer);
}

    #ifdef DEBUG
int
main()
{
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
    #endif

#endif
