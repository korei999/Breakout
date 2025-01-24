#include "ClientSW.hh"

#include "adt/logs.hh"

using namespace adt;

namespace platform::wayland
{

void
ClientSW::start()
{
    m_pDisplay = wl_display_connect(nullptr);
    if (!m_pDisplay)
        throw RuntimeException("wl_display_connect() failed\n");
}

void
ClientSW::disableRelativeMode()
{
}

void
ClientSW::enableRelativeMode()
{
}

void
ClientSW::togglePointerRelativeMode()
{
}

void
ClientSW::toggleFullscreen()
{
}

void
ClientSW::hideCursor()
{
}

void
ClientSW::setCursorImage(String cursorType)
{
}

void
ClientSW::setFullscreen()
{
}

void
ClientSW::unsetFullscreen()
{
}

void
ClientSW::bindGlContext()
{
    //
}

void
ClientSW::unbindGlContext()
{
    //
}

void
ClientSW::setSwapInterval(int interval)
{
}

void
ClientSW::toggleVSync()
{
}

void
ClientSW::swapBuffers()
{
}

void
ClientSW::procEvents()
{
}

void
ClientSW::showWindow()
{
}

void
ClientSW::destroy()
{
    if (m_pDisplay) wl_display_disconnect(m_pDisplay);
}

} /* namespace platform::wayland */
