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

    if (app->m_bPointerRelativeMode || app->m_bRestoreRelativeMode)
    {
        app->m_bRestoreRelativeMode = false;
        app->enableRelativeMode();
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
    for (auto& key : controls::g_aPressedKeys) key = 0;

    if (app->m_bPointerRelativeMode)
    {
        app->m_bRestoreRelativeMode = true;
        app->disableRelativeMode();
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
#ifndef NDEBUG
    if (key >= utils::size(controls::g_aPressedKeys))
    {
        LOG_WARN("key '{}' is out of range\n", key);
        return;
    }
#endif

    controls::g_aPressedKeys[key] = keyState;
    /*LOG_NOTIFY("key: {}, {}\n", key, keyState);*/
}

void
keyboardModifiersHandler(
    [[maybe_unused]] void* data,
    [[maybe_unused]] wl_keyboard* keyboard,
    [[maybe_unused]] u32 serial,
    [[maybe_unused]] u32 modsDepressed,
    [[maybe_unused]] u32 modsLatched,
    [[maybe_unused]] u32 modsLocked,
    [[maybe_unused]] u32 group
)
{
    /* shift == 1, ctrl == 4, alt == 8, super == 64 */
    controls::g_eKeyMods = MOD_STATE(modsDepressed);
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
    s->m_pointerSerial = serial;

    if (s->m_bPointerRelativeMode)
    {
        wl_pointer_set_cursor(pointer, serial, nullptr, 0, 0);
    }
    else
    {
        wl_pointer_set_cursor(
            pointer,
            serial,
            s->m_cursorSurface,
            s->m_cursorImage->hotspot_x,
            s->m_cursorImage->hotspot_y
        );
    }
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
    controls::g_mouse.relX += wl_fixed_to_double(dxUnaccel);
    controls::g_mouse.relY += wl_fixed_to_double(dyUnaccel);
}

} /* namespace input */
} /* namespace wayland */
} /* namespace platform */
