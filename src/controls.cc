#include "controls.hh"

#include "adt/logs.hh"
#include "app.hh"
#include "game.hh"
#include "keybinds.hh"

#include <cmath>

namespace controls
{

bool g_aPressedKeys[300] {};
MOD_STATE g_eKeyMods {};
Mouse g_mouse {};
Camera g_camera {};
bool g_bTTFDebugScreen = false;
bool g_bTTFStepDebug = false;
bool g_bStepDebug = false;
bool g_bPause = false;

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

/* execute commands with 'bRepeat == false' only once per keypress */
template<bool FN_EQUALS_TO(const void* pCommand), u32 ARR_CAP, typename COMMAND_T>
static void
procCommands(Arr<bool, ARR_CAP>* paMap, const COMMAND_T& aCommands)
{
    for (auto& com : aCommands)
    {
        u32 idx = &com - &aCommands[0];
        if (idx >= paMap->getSize())
        {
            LOG_BAD("command array size is bigger than {}, skipping the rest\n", paMap->getSize());
            return;
        }

        if (FN_EQUALS_TO(&com))
        {
            if (com.bRepeat)
            {
                com.exec();
            }
            else
            {
                if (!(*paMap)[idx])
                {
                    (*paMap)[idx] = true;
                    com.exec();
                }
            }
        }
        else
        {
            (*paMap)[idx] = false;
        }
    }
}

void
procKeys()
{
    game::playerEntity().dir = {};

    {
        /* 500 should be enough */
        static Arr<bool, MAX_COMMANDS> aPressedKeysOnceMap {};
        aPressedKeysOnceMap.setSize(MAX_COMMANDS);

        procCommands<
            [](const void* pCommand) -> bool {
                const auto* pCom = (keybinds::Command*)pCommand;
                return g_aPressedKeys[pCom->key];
            }>
        (
            &aPressedKeysOnceMap,
            keybinds::inl_aCommands
        );
    }

    /* apply ModCommands after */
    {
        static Arr<bool, MAX_COMMANDS> aPressedModsOnceMap {};
        aPressedModsOnceMap.setSize(MAX_COMMANDS);

        procCommands<
            [](const void* pArg) -> bool {
                const auto* pCom = (keybinds::ModCommand*)pArg;
                return g_eKeyMods == pCom->eMod;
            }>
        (
            &aPressedModsOnceMap,
            keybinds::inl_aModCommands
        );
    }
}

void
updateView()
{
    g_camera.view = math::M4LookAt(g_camera.pos, g_camera.pos + g_camera.front, g_camera.up);
}

void
togglePause()
{
    utils::toggle(&app::g_pWindow->m_bPaused);
    LOG_WARN("paused: {}\n", app::g_pWindow->m_bPaused);
}

void
move(math::V2 dir)
{
    game::playerEntity().dir = dir;
}

void
mulDirection(f32 factor)
{
    game::playerEntity().dir *= factor;
}

void
toggleMouseLock()
{
    app::g_pWindow->togglePointerRelativeMode();
}

void
quit()
{
    app::g_pWindow->m_bRunning = false;
    app::g_pMixer->m_bRunning = false;

    LOG_OK("quit...\n");
}

void
toggleFullscreen()
{
    app::g_pWindow->toggleFullscreen();
}

void
toggleVSync()
{
    app::g_pWindow->toggleVSync();
}

void
releaseBall()
{
    auto& enPlayer = game::playerEntity();
    auto& enBall = game::g_aEntities[game::g_ball.enIdx];

    utils::toggle(&game::g_ball.bReleased);
    enBall.dir = math::V2{0.0f, 1.0f} + math::V2{enPlayer.dir * 0.25f};
}

void
toggleDebugScreen()
{
    g_bTTFDebugScreen = !g_bTTFDebugScreen;
    LOG_NOTIFY("g_bTTFDebugScreen: {}\n", g_bTTFDebugScreen);
}

void
toggleStepDebug()
{
    utils::toggle(&g_bStepDebug);
    LOG_NOTIFY("g_bStepDebug: {}\n", g_bStepDebug);
}

} /* namespace controls */
