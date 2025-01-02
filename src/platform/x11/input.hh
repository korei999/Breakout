#pragma once

#include "X11Window.hh"

namespace platform
{
namespace x11
{
namespace input
{

void mapX11KeycodesToLinuxKeycodes(Win* s);
void storeAtoms(Win* s);
void procEvents(Win* s);

} /* namespace input */
} /* namespace x11 */
} /* namespace platform */
