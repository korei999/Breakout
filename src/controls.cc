#include "controls.hh"

#include "logs.hh"
#include "app.hh"

#include <math.h>

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
                app::g_pWindow->bPaused = !app::g_pWindow->bPaused;
                if (app::g_pWindow->bPaused) LOG_WARN("paused: %d\n", app::g_pWindow->bPaused);
            }
            break;

        case KEY_Q:
            if (pressed) WindowTogglePointerRelativeMode(app::g_pWindow);
            break;

        case KEY_ESC:
        case KEY_CAPSLOCK:
            if (pressed)
            {
                app::g_pWindow->bRunning = false;

#ifdef _WIN32
                /* FIXME: implement mixer on windows */
                app::g_pMixer->bRunning = false;
#endif

                LOG_OK("quit...\n");
            }
            break;

        case KEY_R:
            /*if (pressed) incCounter = 0;*/
            break;

        case KEY_F:
            if (pressed) WindowToggleFullscreen(app::g_pWindow);
            break;

        case KEY_V:
            if (pressed) WindowToggleVSync(app::g_pWindow);
            break;

        case KEY_SPACE:
            {
                if (!pressed) break;

                game::g_ball.bReleased = !game::g_ball.bReleased;
                game::g_ball.dir = math::V3(0.0f, 1.0f, 0.0f) + game::g_player.dir * 0.25f;
            }
            break;

        default:
            break;
    }
}

void
procKeys()
{
    PlayerProcMovements(&game::g_player);
}

static void
PlayerProcMovements(game::Player* s)
{
    s->dir = {};

    if (g_pressedKeys[KEY_A])
    {
        s->dir = {-1.0f, 0.0f};
    }

    if (g_pressedKeys[KEY_D])
    {
        s->dir = {1.0f, 0.0f};
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
updateView()
{
    g_camera.view = math::M4LookAt(g_camera.pos, g_camera.pos + g_camera.front, g_camera.up);
}

void
updateProj(game::Player* s, f32 fov, f32 aspect, f32 near, f32 far)
{
    g_camera.proj = math::M4Pers(fov, aspect, near, far);
}

} /* namespace controls */
