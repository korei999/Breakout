#include "app.hh"

namespace app
{

adt::ThreadPool* g_pThreadPool;

int g_argc = 0;
char** g_argv = nullptr;

audio::IMixer* g_pMixer;
IWindow* g_pWindow;

} /* namespace app */
