#pragma once

#include "audio.hh"
#include "IWindow.hh"
#include "adt/ThreadPool.hh"

namespace app
{

extern adt::ThreadPool* g_pThreadPool;

extern int g_argc;
extern char** g_argv;

extern audio::IMixer* g_pMixer;
extern IWindow* g_pWindow;

audio::IMixer* platformMixerAlloc(IAllocator* pAlloc);
IWindow* platformWindowAlloc(IAllocator* pAlloc);

} /* namespace app */
