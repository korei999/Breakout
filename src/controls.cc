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
bool g_bStepDebug = false;
bool g_bPause = false;
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
    auto& enPlayer = game::g_aEntities[game::g_player.enIdx];
    auto& enBall = game::g_aEntities[game::g_ball.enIdx];

    switch (key)
    {
        case KEY_P:
        case KEY_GRAVE:
            if (pressed)
            {
                utils::toggle(&app::g_pWindow->bPaused);
                LOG_WARN("paused: {}\n", app::g_pWindow->bPaused);
            } break;

        case KEY_Q:
            if (pressed) app::g_pWindow->togglePointerRelativeMode();
            break;

        case KEY_ESC:
            if (pressed)
            {
                app::g_pWindow->bRunning = false;
                app::g_pMixer->bRunning = false;

                LOG_OK("quit...\n");
            } break;

        case KEY_F:
            if (pressed) app::g_pWindow->toggleFullscreen();
            break;

        case KEY_V:
            if (pressed) app::g_pWindow->toggleVSync();
            break;

        case KEY_SPACE: {
            if (!pressed) break;

            utils::toggle(&game::g_ball.bReleased);
            enBall.dir = math::V2{0.0f, 1.0f} + math::V2{enPlayer.dir * 0.25f};
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

        case KEY_B: {
            if (!pressed) break;

            utils::toggle(&g_bStepDebug);
            LOG_NOTIFY("g_bStepDebug: {}\n", g_bStepDebug);
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
    auto& enPlayer = game::g_aEntities[game::g_player.enIdx];
    enPlayer.dir = {};

    if (g_aPressedKeys[KEY_A]) enPlayer.dir = {-1.0f, 0.0f};
    if (g_aPressedKeys[KEY_D]) enPlayer.dir = {1.0f, 0.0f};
    if (g_aPressedKeys[KEY_LEFTALT]) enPlayer.dir /= 2.0f;
    if (g_aPressedKeys[KEY_LEFTSHIFT]) enPlayer.dir *= 2.0f;
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
