#pragma once

#include "Model.hh"
#include "adt/Pair.hh"

namespace frame
{

constexpr u32 ASSET_MAX_COUNT = 512;

constexpr f32 WIDTH = 1000.0f;
constexpr f32 HEIGHT = 1000.0f;

extern Pair<f32, f32> g_unit;

extern f32 g_uiWidth;
extern f32 g_uiHeight;

extern long g_currTime;
extern f64 g_deltaTime;
extern long g_lastTime;

extern f64 g_currDrawTime;
extern f64 g_frameTime;
extern f64 g_lastFrameTime;

extern f64 g_prevTime;
extern int g_nfps;

extern Ubo g_uboProjView;

void run();

} /* namespace frame */
