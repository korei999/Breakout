#pragma once

#include "adt/types.hh"
#include "colors.hh"

using namespace adt;

namespace game
{

enum class STATE : u8 {
    ACTIVE,
    MENU,
    WIN
};

enum class BLOCK_COLOR : s8 {
    INVISIBLE = -1, GRAY, WHITE, RED, GREEN, BLUE
};

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

struct Ball
{
    bool bReleased;
    f32 speed;
    f32 radius;
    math::V2 pos;
    math::V2 dir;
};

constexpr math::V3
blockColorToV3(BLOCK_COLOR col)
{
    constexpr math::V3 map[] {
        colors::black, colors::darkGrey, colors::white, colors::red, colors::green, colors::blue
    };

    return map[int(col) + 1];
}

inline const s8 g_level0[][15] {
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
    { -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
};

inline const s8 g_level1[][15] {
    { 4,  4,  4,  4,  4,  4, -1, -1, -1,  4,  4,  4,  4,  4,  4},
    { 3,  3,  3,  3,  3,  3, -1, -1, -1,  3,  3,  3,  3,  3,  3},
    { 2,  2,  2,  2,  2,  2, -1, -1, -1,  2,  2,  2,  2,  2,  2},
    { 1,  1,  1,  1,  1,  1, -1, -1, -1,  1,  1,  1,  1,  1,  1},
    { 2,  3,  1,  2,  3,  2, -1, -1, -1,  4,  2,  1,  3,  2,  2},
    { 3,  3,  3,  3,  3,  3, -1, -1, -1,  3,  3,  3,  3,  3,  3},
    { 2,  2,  2,  2,  2,  2, -1, -1, -1,  2,  2,  2,  2,  2,  2},
    { 1,  1,  1,  1,  1,  1, -1, -1, -1,  1,  1,  1,  1,  1,  1},
    { 0,  0,  4,  3,  1,  2, -1, -1, -1,  3,  2,  4,  2,  1,  2},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

} /* namespace game */
