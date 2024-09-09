#pragma once

#include "adt/Array.hh"
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

enum class COLOR : u8 {
    INVISIBLE = 0, DIMGRAY, GRAY, WHITE, RED, GREEN, BLUE
};

struct Entity
{
    math::V2 pos;
    u16 shaderIdx;
    u16 modelIdx;
    u16 texIdx;
    game::COLOR color;
    bool bDead;
};

inline u32
GetNextEntityIdx(Array<Entity>* s)
{
    ArrayPush(s, {});
    return s->size - 1;
}

struct Player
{
    u32 idxEntity = 0;
    math::V3 pos {0, 0, 3};
    f64 speed = 5.0;
    math::V3 dir {};
};

struct Projectile
{
    u16 idxEntity;
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
blockColorToV3(COLOR col)
{
    constexpr math::V3 map[] {
        {}, colors::dimGrey, colors::darkGrey, colors::white, colors::red, colors::green, colors::blue
    };

    return map[int(col)];
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
    { 4,  4,  4,  4,  4,  4,  0,  0,  0,  6,  4,  4,  4,  4,  4},
    { 3,  3,  3,  3,  3,  5,  0,  0,  0,  4,  3,  3,  3,  3,  3},
    { 3,  4,  3,  2,  2,  6,  0,  0,  0,  2,  2,  4,  2,  2,  5},
    { 2,  2,  4,  3,  2,  3,  2,  0,  0,  3,  2,  4,  2,  4,  5},
    { 5,  4,  2,  2,  3,  3,  0,  0,  0,  4,  2,  2,  5,  2,  3},
    { 3,  6,  3,  3,  3,  4,  0,  0,  0,  3,  3,  3,  4,  3,  2},
    { 2,  2,  2,  2,  3,  5,  1,  0,  0,  2,  2,  4,  2,  2,  4},
    { 4,  3,  2,  3,  4,  4,  0,  0,  0,  2,  2,  2,  5,  3,  3},
    { 3,  2,  4,  3,  2,  2,  0,  0,  0,  3,  2,  4,  2,  2,  2},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
};

} /* namespace game */
