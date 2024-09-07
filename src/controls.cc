#include <math.h>

#include "adt/logs.hh"
#include "controls.hh"
#include "frame.hh"

#ifdef __linux__
    #include <linux/input-event-codes.h>
#elif _WIN32
#endif

namespace controls
{

bool g_pressedKeys[300] {};

void
PlayerProcMouse(Player* s)
{
    f64 offsetX = (s->mouse.relX - s->mouse.prevRelX) * s->mouse.sens;
    f64 offsetY = (s->mouse.prevRelY - s->mouse.relY) * s->mouse.sens;

    s->mouse.prevRelX = s->mouse.relX;
    s->mouse.prevRelY = s->mouse.relY;

    s->mouse.yaw += offsetX;
    s->mouse.pitch += offsetY;

    if (s->mouse.pitch > 89.9)
        s->mouse.pitch = 89.9;
    if (s->mouse.pitch < -89.9)
        s->mouse.pitch = -89.9;

    s->front = math::V3Norm({
        f32(cos(math::toRad(s->mouse.yaw)) * cos(math::toRad(s->mouse.pitch))),
        f32(sin(math::toRad(s->mouse.pitch))),
        f32(sin(math::toRad(s->mouse.yaw)) * cos(math::toRad(s->mouse.pitch)))
    });

    s->right = V3Norm(V3Cross(s->front, s->up));
}

void
procKeysOnce(u32 key, u32 pressed)
{
    switch (key)
    {
        case KEY_P:
        case KEY_GRAVE:
            if (pressed)
            {
                frame::g_pApp->bPaused = !frame::g_pApp->bPaused;
                if (frame::g_pApp->bPaused) LOG_WARN("paused: %d\n", frame::g_pApp->bPaused);
            }
            break;

        case KEY_Q:
            if (pressed) AppTogglePointerRelativeMode(frame::g_pApp);
            break;

        case KEY_ESC:
        case KEY_CAPSLOCK:
            if (pressed)
            {
                frame::g_pApp->bRunning = false;

#ifdef _WIN32
                /* FIXME: implement mixer on windows */
                frame::g_pMixer->bRunning = false;
#endif

                LOG_OK("quit...\n");
            }
            break;

        case KEY_R:
            /*if (pressed) incCounter = 0;*/
            break;

        case KEY_F:
            if (pressed) AppToggleFullscreen(frame::g_pApp);
            break;

        case KEY_V:
            if (pressed) AppToggleVSync(frame::g_pApp);
            break;

        case KEY_SPACE:
            {
                if (!pressed) break;

                frame::g_ball.bReleased = !frame::g_ball.bReleased;
                frame::g_ball.dir = math::V3(0.0f, 1.0f, 0.0f) + frame::g_player.dir * 0.25f;
            }
            break;

        default:
            break;
    }
}

void
PlayerProcKeys(Player* s)
{
    PlayerProcMovements(s);
}

void
PlayerProcMovements(Player* s)
{
    s->dir = {0.0f, 0.0f, 0.0f};

    if (g_pressedKeys[KEY_A])
    {
        s->dir = {-1.0f, 0.0f, 0.0f};
    }

    if (g_pressedKeys[KEY_D])
    {
        s->dir = {1.0f, 0.0f, 0.0f};
    }

    if (g_pressedKeys[KEY_LEFTALT])
    {
        s->dir /= 2.0;
    }

    if (g_pressedKeys[KEY_LEFTSHIFT])
    {
        s->dir *= 2.0;
    }
}

void 
PlayerUpdateView(Player* s)
{
    s->view = math::M4LookAt(s->pos, s->pos + s->front, s->up);
}

void 
PlayerUpdateProj(Player* s, f32 fov, f32 aspect, f32 near, f32 far)
{
    s->proj = math::M4Pers(fov, aspect, near, far);
}

} /* namespace controls */
