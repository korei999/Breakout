#include "controls.hh"

#include "app.hh"
#include "adt/logs.hh"

#include <cmath>

namespace controls
{

bool g_aPressedKeys[300] {};
Mouse g_mouse {};
Camera g_camera {};
bool g_bTTFDebugScreen = false;
bool g_bTTFDebugDots = false;
bool g_bTTFStepDebug = false;
int g_nDots = 0;

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
        f32(std::cos(math::toRad(g_mouse.yaw)) * std::cos(math::toRad(g_mouse.pitch))),
        f32(std::sin(math::toRad(g_mouse.pitch))),
        f32(std::sin(math::toRad(g_mouse.yaw)) * std::cos(math::toRad(g_mouse.pitch)))
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
            } break;

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
            } break;

        case KEY_F:
            if (pressed) WindowToggleFullscreen(app::g_pWindow);
            break;

        case KEY_V:
            if (pressed) WindowToggleVSync(app::g_pWindow);
            break;

        case KEY_SPACE: {
            if (!pressed) break;

            game::g_ball.bReleased = !game::g_ball.bReleased;
            game::g_ball.dir = math::V2{0.0f, 1.0f} + math::V2{game::g_player.dir * 0.25f};
        } break;

        case KEY_T: {
            if (!pressed) break;

            g_bTTFDebugScreen = !g_bTTFDebugScreen;
            LOG_NOTIFY("g_bTTFDebugScreen: {}\n", g_bTTFDebugScreen);
        } break;

        case KEY_R: {
            if (!pressed) break;

            g_bTTFDebugDots = !g_bTTFDebugDots;
            LOG_NOTIFY("g_bTTFDebugDots: {}\n", g_bTTFDebugDots);
        } break;

        case KEY_E: {
            if (!pressed) break;

            if (g_aPressedKeys[KEY_LEFTSHIFT]) --g_nDots;
            else if (g_aPressedKeys[KEY_LEFTCTRL]) g_nDots += 9;
            else if (g_aPressedKeys[KEY_LEFTALT]) g_nDots -= 9;
            else ++g_nDots;

            /*LOG_NOTIFY("g_nDots: {}\n", g_nDots);*/
        } break;

        case KEY_G: {
            if (!pressed) break;

            g_bTTFStepDebug = !g_bTTFStepDebug;
            LOG_NOTIFY("g_bTTFStepDebbug: {}\n", g_bTTFStepDebug);
        } break;

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

    if (g_aPressedKeys[KEY_A])
    {
        s->dir = {-1.0f, 0.0f};
    }

    if (g_aPressedKeys[KEY_D])
    {
        s->dir = {1.0f, 0.0f};
    }

    if (g_aPressedKeys[KEY_LEFTALT])
    {
        s->dir /= 2.0;
    }

    if (g_aPressedKeys[KEY_LEFTSHIFT])
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
