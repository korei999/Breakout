#include "app.hh"

namespace app
{

#ifdef __linux__
platform::pipewire::Mixer* g_pMixer;
platform::wayland::Client* g_pWindow;
#elif _WIN32
platform::win32::Mixer* g_pMixer;
platform::win32::Win32Window* g_pWindow;
#endif

} /* namespace app */
