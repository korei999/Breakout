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
Mouse g_mouse {};
Camera g_camera {};

static void PlayerProcMovements(game::Player* s);

void
procMouse()
{
    f64 offsetX = (g_mouse.relX - g_mouse.prevRelX) * g_mouse.sens;
    f64 offsetY = (g_mouse.prevRelY - g_mouse.relY) * g_mouse.sens;

    g_mouse.prevRelX = g_mouse.relX;
    g_mouse.prevRelY = g_mouse.relY;

    g_mouse.yaw += offsetX;
    g_mouse.pitch += offsetY;

    if (g_mouse.pitch > 89.9)
        g_mouse.pitch = 89.9;
    if (g_mouse.pitch < -89.9)
        g_mouse.pitch = -89.9;

    g_camera.front = math::V3Norm({
        f32(cos(math::toRad(g_mouse.yaw)) * cos(math::toRad(g_mouse.pitch))),
        f32(sin(math::toRad(g_mouse.pitch))),
        f32(sin(math::toRad(g_mouse.yaw)) * cos(math::toRad(g_mouse.pitch)))
    });

    g_camera.right = V3Norm(V3Cross(g_camera.front, g_camera.up));
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
procKeys()
{
    PlayerProcMovements(&frame::g_player);
}

static void
PlayerProcMovements(game::Player* s)
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
PlayerUpdateView(game::Player* s)
{
    // s->view = math::M4LookAt(s->pos, s->pos + s->front, s->up);
}

void 
PlayerUpdateProj(game::Player* s, f32 fov, f32 aspect, f32 near, f32 far)
{
    // s->proj = math::M4Pers(fov, aspect, near, far);
}

} /* namespace controls */
