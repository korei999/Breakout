#pragma once

#include "adt/Pair.hh"
#include "game.hh"

using namespace adt;

namespace frame
{

constexpr u32 ASSET_MAX_COUNT = 512;

constexpr f32 WIDTH = 1000.0f;
constexpr f32 HEIGHT = 1000.0f;

extern game::Player g_player;
extern game::Ball g_ball;

extern Pair<f32, f32> g_unit;

extern f32 g_fov;
extern f32 g_uiWidth;
extern f32 g_uiHeight;

extern f64 g_currTime;
extern f64 g_deltaTime;
extern f64 g_lastFrameTime;

void run();

} /* namespace frame */
