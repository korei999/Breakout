#pragma once

#include "adt/Array.hh"
#include "adt/types.hh"
#include "colors.hh"

#include <assert.h>

using namespace adt;

namespace game
{

constexpr u32 ASSET_MAX_COUNT = 512;

enum REFLECT_SIDE : s8
{
    NONE = -1, UP, RIGHT, DOWN, LEFT, ESIZE
};

enum class STATE : u8
{
    ACTIVE,
    MENU,
    WIN
};

enum class COLOR : u8
{
    INVISIBLE = 0,
    DIMGRAY,
    GRAY,
    WHITE,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    TEAL,
    ORANGERED,
    ESIZE
};

struct Entity
{
    math::V2 pos;
    f32 width;
    f32 height;
    f32 xOff;
    f32 yOff;
    u16 shaderIdx;
    u16 texIdx;
    game::COLOR eColor;
    bool bDead;
};

struct Player
{
    Entity base;
    f32 speed = 5.0;
    math::V2 dir {};
};

struct Ball
{
    Entity base;
    bool bReleased;
    f32 speed;
    f32 radius;
    math::V2 dir;
};

void loadThings();
void loadLevel();
void updateGame();
void drawFPSCounter(Allocator* pAlloc);
void drawEntities();
void cleanup();

constexpr math::V3
blockColorToV3(COLOR col)
{
    using namespace colors;

    constexpr colors::IDX map[u64(COLOR::ESIZE)] {
        {},
        IDX::DIMGREY,
        IDX::DARKGREY,
        IDX::WHITE,
        IDX::RED,
        IDX::GREEN,
        IDX::BLUE,
        IDX::YELLOW,
        IDX::TEAL,
        IDX::ORANGERED
    };

    assert(col < COLOR::ESIZE);

    return colors::get(map[int(col)]);
}

constexpr s8 g_level0[][15] {
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
};

constexpr s8 g_level1[][15] {
    { 4,  4,  6,  7,  4,  4,  2,  2,  3,  6,  4,  4,  6,  4,  4},
    { 3,  3,  3,  3,  3,  5,  0,  0,  0,  4,  3,  3,  4,  3,  3},
    { 3,  4,  3,  2,  2,  6,  0,  0,  0,  2,  2,  5,  3,  2,  5},
    { 2,  2,  4,  3,  2,  3,  2,  0,  0,  3,  2,  4,  2,  4,  6},
    { 5,  4,  2,  2,  3,  3,  0,  0,  0,  4,  2,  2,  5,  2,  6},
    { 3,  6,  3,  3,  3,  4,  0,  0,  0,  3,  3,  6,  4,  3,  2},
    { 2,  2,  2,  2,  3,  5,  2,  0,  0,  2,  2,  6,  2,  2,  4},
    { 4,  3,  2,  3,  4,  4,  0,  0,  0,  2,  2,  7,  5,  7,  3},
    { 3,  2,  4,  3,  2,  2,  0,  0,  0,  3,  2,  4,  2,  2,  2},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
};

extern Player g_player;
extern Ball g_ball;
extern Array<game::Entity*> g_aPEntities;

} /* namespace game */
