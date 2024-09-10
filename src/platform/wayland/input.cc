#include "input.hh"

#include "Client.hh"
#include "adt/logs.hh"
#include "adt/utils.hh"
#include "controls.hh"

namespace platform
{
namespace wayland
{
namespace input
{

void
keyboardKeymapHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 format,
    [[maybe_unused]] s32 fd,
    [[maybe_unused]] u32 size
)
{
    //
}

void
keyboardEnterHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface,
    [[maybe_unused]] wl_array* keys
)
{
    LOG_OK("keyboardEnterHandler\n");

    auto app = (Client*)(data);

    if (app->base.bPointerRelativeMode || app->bRestoreRelativeMode)
    {
        app->bRestoreRelativeMode = false;
        ClientEnableRelativeMode(app);
    }
}

void
keyboardLeaveHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface
)
{
    LOG_OK("keyboardLeaveHandler\n");

    auto app = (Client*)(data);

    /* prevent keys getting stuck after leaving surface */
    for (auto& key : controls::g_pressedKeys) key = 0;

    if (app->base.bPointerRelativeMode)
    {
        app->bRestoreRelativeMode = true;
        ClientDisableRelativeMode(app);
    }
}

void
keyboardKeyHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] u32 key,
    [[maybe_unused]] u32 keyState
)
{
#ifdef DEBUG
    if (key >= utils::size(controls::g_pressedKeys))
    {
        LOG_WARN("key '%u' is out of range\n", key);
        return;
    }
#endif

    controls::g_pressedKeys[key] = keyState;
    controls::procKeysOnce(key, keyState);
}

void
keyboardModifiersHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] u32 modsDepressed,
    [[maybe_unused]] u32 modsLatched, [[maybe_unused]] u32 modsLocked,
    [[maybe_unused]] u32 group
)
{
    //
}

void
keyboardRepeatInfoHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* wl_keyboard,
    [[maybe_unused]] s32 rate,
    [[maybe_unused]] s32 delay
)
{
    //
}


void
pointerEnterHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_pointer* pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface,
    [[maybe_unused]] wl_fixed_t surfaceX,
    [[maybe_unused]] wl_fixed_t surfaceY
)
{
    LOG_OK("pointerEnterHandler\n");

    auto s = (Client*)(data);
    s->_pointerSerial = serial;

    if (s->base.bPointerRelativeMode)
        wl_pointer_set_cursor(pointer, serial, nullptr, 0, 0);
    else
        wl_pointer_set_cursor(
            pointer,
            serial,
            s->cursorSurface,
            s->cursorImage->hotspot_x,
            s->cursorImage->hotspot_y
        );
}

void
pointerLeaveHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_pointer* pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] wl_surface* surface
)
{
    LOG_OK("pointerLeaveHandler\n");
}

void
pointerMotionHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_pointer* pointer,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] wl_fixed_t surfaceX,
    [[maybe_unused]] wl_fixed_t surfaceY
)
{
    controls::g_mouse.absX = wl_fixed_to_double(surfaceX);
    controls::g_mouse.absY = wl_fixed_to_double(surfaceY);

    /*auto s = (Client*)(data);*/
    /*if (s->base.bHideCursor)*/
    /*    ClientHideCursor(s);*/
}

void
pointerButtonHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_pointer* pointer,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] u32 button,
    [[maybe_unused]] u32 buttonState
)
{
    controls::g_mouse.button = button;
    controls::g_mouse.state = buttonState;
}

void
pointerAxisHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_pointer* pointer,
    [[maybe_unused]] u32 time,
    [[maybe_unused]] u32 axis,
    [[maybe_unused]] wl_fixed_t value
)
{
    //
}

void
relativePointerMotionHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] zwp_relative_pointer_v1* zwp_relative_pointer_v1,
    [[maybe_unused]] u32 utime_hi,
    [[maybe_unused]] u32 utime_lo,
    [[maybe_unused]] wl_fixed_t dx,
    [[maybe_unused]] wl_fixed_t dy,
    [[maybe_unused]] wl_fixed_t dxUnaccel,
    [[maybe_unused]] wl_fixed_t dyUnaccel
)
{
    controls::g_mouse.relX += wl_fixed_to_int(dxUnaccel);
    controls::g_mouse.relY += wl_fixed_to_int(dyUnaccel);
}

} /* namespace input */
} /* namespace wayland */
} /* namespace platform */
