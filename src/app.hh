#pragma once

#include "audio.hh"
#include "IWindow.hh"
#include "adt/ThreadPool.hh"

namespace app
{

enum class WINDOW_TYPE : adt::u8 { X11_GL, WAYLAND_GL, WAYLAND_SW, WIN11_GL };

extern WINDOW_TYPE g_eWindowType;

extern adt::ThreadPool* g_pThreadPool;

extern int g_argc;
extern char** g_argv;

extern audio::IMixer* g_pMixer;
extern IWindow* g_pWindow;

audio::IMixer* platformMixerAlloc(IAllocator* pAlloc);
IWindow* platformWindowAlloc(IAllocator* pAlloc);

} /* namespace app */
