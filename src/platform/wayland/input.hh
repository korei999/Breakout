#pragma once
#include "adt/types.hh"
#include "wayland-protocols/relative-pointer-unstable-v1.h"

#include <linux/input-event-codes.h>

using namespace adt;

namespace platform
{
namespace wayland
{
namespace input
{

void keyboardKeymapHandler(void* data, wl_keyboard* keyboard, u32 format, s32 fd, u32 size);
void keyboardEnterHandler(void* data, wl_keyboard* keyboard, u32 serial, wl_surface* surface, wl_array* keys);
void keyboardLeaveHandler(void* data, wl_keyboard* keyboard, u32 serial, wl_surface* surface);
void keyboardKeyHandler(void* data, wl_keyboard* keyboard, u32 serial, u32 time, u32 key, u32 state);
void keyboardModifiersHandler(void* data, wl_keyboard* keyboard, u32 serial, u32 modsDepressed, u32 modsLatched, u32 modsLocked, u32 group);
void keyboardRepeatInfoHandler(void* data, wl_keyboard* keyboard, s32 rate, s32 delay);

void pointerEnterHandler(void* data, wl_pointer* pointer, u32 serial, wl_surface* surface, wl_fixed_t surfaceX, wl_fixed_t surfaceY);
void pointerLeaveHandler(void* data, wl_pointer* pointer, u32 serial, wl_surface* surface);
void pointerMotionHandler(void* data, wl_pointer* pointer, u32 time, wl_fixed_t surfaceX, wl_fixed_t surfaceY);
void pointerButtonHandler(void* data, wl_pointer* pointer, u32 serial, u32 time, u32 button, u32 state);
void pointerAxisHandler(void* data, wl_pointer* pointer, u32 time, u32 axis, wl_fixed_t value);

void relativePointerMotionHandler(void* data, zwp_relative_pointer_v1* zwp_relative_pointer_v1, u32 utime_hi, u32 utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel);

} /* namespace input */
} /* namespace wayland */
} /* namespace platform */
