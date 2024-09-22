#pragma once

#ifdef __linux__
    #include "platform/wayland/Client.hh"
    #include "platform/pipewire/Mixer.hh"
#elif _WIN32
    #include "platform/win32/Mixer.hh"
    #include "platform/win32/Win32Window.hh"
#endif

namespace app
{

#ifdef __linux__
extern platform::pipewire::Mixer* g_pMixer;
extern platform::wayland::Client* g_pWindow;
#elif _WIN32
extern platform::win32::Mixer* g_pMixer;
extern platform::win32::Win32Window* g_pWindow;
#endif

} /* namespace app */
