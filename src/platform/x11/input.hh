#pragma once

#include "X11Window.hh"

namespace platform
{
namespace x11
{
namespace input
{

void mapX11KeycodesToLinuxKeycodes(Window* s);
void storeAtoms(Window* s);
void procEvents(Window* s);

} /* namespace input */
} /* namespace x11 */
} /* namespace platform */
