#pragma once
#include "Win32Window.hh"

namespace platform
{
namespace win32
{
namespace input
{

void registerRawMouseDevice(Win32Window* self, bool on);
void registerRawKBDevice(Win32Window* self, bool on);
bool enterFullscreen(HWND hwnd, int fullscreenWidth, int fullscreenHeight, int colourBits, int refreshRate);
bool exitFullscreen(HWND hwnd, int windowX, int windowY, int windowedWidth, int windowedHeight, int windowedPaddingX, int windowedPaddingY);
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} /* namespace input */
} /* namespace win32 */
} /* namespace platform */
