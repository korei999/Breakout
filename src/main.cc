#include "adt/Arena.hh"
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

    platform::pipewire::MixerInit(&mixer, argc, argv);

    frame::g_pApp = &app.base;
    frame::g_pMixer = &mixer.base;

    frame::run();

    platform::pipewire::MixerDestroy(&mixer);
    platform::wayland::ClientDestroy(&app);
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
    using namespace platform;

    win32::Mixer mixer(&alMixer.base);
    win32::MixerInit(&mixer, {}, {});
    win32::Window app("Breakout", instance);

    frame::g_pMixer = &mixer.base;
    frame::g_pApp = &app.base;

    frame::run();

    win32::MixerDestroy(&mixer);
    win32::WindowDestroy(&app);
}

    #ifdef DEBUG
int
main()
{
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
    #endif

#endif
