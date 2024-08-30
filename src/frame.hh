#pragma once

#include "App.hh"
#include "adt/Pair.hh"
#include "adt/Queue.hh"
#include "controls.hh"
#include "game.hh"

using namespace adt;

namespace frame
{

struct Entity
{
    math::V2 pos;
    u16 shaderIdx;
    u16 modelIdx;
    u16 texIdx;
    game::BLOCK_COLOR color;
    bool bDead;
};

struct Projectile
{
    u16 entityIdx;
    f32 speed;
    math::V2 pos;
    math::V2 dir;
    bool bBroken;
};

constexpr u32 ASSET_MAX_COUNT = 512;

constexpr f32 WIDTH = 1000.0f;
constexpr f32 HEIGHT = 1000.0f;

extern App* g_pApp;

extern controls::Player g_player;

extern Pair<f32, f32> g_unit;

extern f32 g_fov;
extern f32 g_uiWidth;
extern f32 g_uiHeight;

extern f64 g_currTime;
extern f64 g_deltaTime;
extern f64 g_lastFrameTime;

extern Queue<Projectile> g_projectiles;

void run();

} /* namespace frame */
