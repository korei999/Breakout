#pragma once

#include "audio.hh"
#include "Window.hh"

namespace app
{

extern int g_argc;
extern char** g_argv;

extern audio::Mixer* g_pMixer;
extern Window* g_pWindow;

} /* namespace app */
