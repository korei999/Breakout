#include "frame.hh"

using namespace adt;

#ifdef __linux__
    #include "platform/wayland/Client.hh"

int
main()
{
    platform::wayland::Client app("WlCubes");

    frame::g_pApp = &app.base;
    frame::run();

    platform::wayland::ClientDestroy(&app);
}

#elif _WIN32
    #include "platform/windows/windows.hh"

int WINAPI
WinMain([[maybe_unused]] HINSTANCE instance,
        [[maybe_unused]] HINSTANCE previnstance,
        [[maybe_unused]] LPSTR cmdline,
        [[maybe_unused]] int cmdshow)
{
    win32::Window app("wl-cube", instance);
    frame::g_app = &app;
    frame::run();

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
