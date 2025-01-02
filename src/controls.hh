#pragma once

#include "keys.hh"
#include "adt/math.hh"

using namespace adt;

namespace controls
{

constexpr u32 MAX_COMMANDS = 500;

struct Mouse
{
    f64 prevAbsX = 0;
    f64 prevAbsY = 0;

    f64 prevRelX = 0;
    f64 prevRelY = 0;

    f64 absX = 0;
    f64 absY = 0;

    f64 relX = 0;
    f64 relY = 0;

    f64 sens = 0.05;
    f64 yaw = -90.0;
    f64 pitch = 0;

    u32 button = 0;
    u32 state = 0;
};

struct Camera
{
    /* proj and view are adjecent for nice ubo buffering */
    math::M4 proj {};
    math::M4 view {};

    math::V3 pos {0, 0, 3};
    math::V3 front {0, 0, -1};
    math::V3 right {1, 0, 0};
    static constexpr math::V3 up {0, 1, 0};
};

extern bool g_aPressedKeys[300];
extern MOD_STATE g_eKeyMods;
extern Mouse g_mouse;
extern Camera g_camera;
extern bool g_bTTFDebugScreen;
extern bool g_bTTFStepDebug;
extern bool g_bStepDebug;

void procMouse();
void procKeys();
void updateView();

/* Commands */

void togglePause();
void move(math::V2 dir);
void mulDirection(f32 factor);
void toggleMouseLock();
void quit();
void toggleFullscreen();
void toggleVSync();
void releaseBall();
void toggleDebugScreen();
void toggleStepDebug();

} /* namespace controls */
