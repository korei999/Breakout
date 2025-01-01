#pragma once

#include "adt/Arena.hh"
#include "adt/Pool.hh"
#include "adt/types.hh"
#include "colors.hh"

#include <cassert>

namespace game
{

using namespace adt;

constexpr u32 ASSET_MAX_COUNT = 256;
constexpr u32 TICK_RATE = 240;
constexpr f64 FIXED_DELTA_TIME = 1.0 / f64(TICK_RATE);

enum REFLECT_SIDE : s8
{
    NONE = -1, UP, RIGHT, DOWN, LEFT, CORNER, ELAST
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

enum ENTITY_TYPE : u8
{
    GEN, PLAYER, BALL
};

struct Entity
{
    ENTITY_TYPE eType {};
    math::V2 pos {};
    math::V2 dir {};
    /*math::V2 vel {};*/
    f32 speed {};
    f32 width {};
    f32 height {};
    f32 xOff {};
    f32 yOff {};
    f32 zOff {};
    u16 shaderIdx {};
    u16 texIdx {};
    game::COLOR eColor {};
    bool bDead {};
    bool bRemoveAfterDraw {};
};

struct Player
{
    u16 enIdx {};
};

struct Ball
{
    u16 enIdx {};
    f32 radius {};
    bool bReleased {};
    bool bCollided {};
};

struct Block
{
    u16 enIdx {};
};

struct Level
{
    u32 width;
    u32 height;
    s8* aTiles;
};

void loadAssets();
void loadLevel();
void updateState();
void draw(Arena* pAlloc, const f64 alpha);
void cleanup();

constexpr math::V3
blockColorToV3(COLOR col)
{
    using namespace colors;

    constexpr colors::IDX map[u64(COLOR::ESIZE)] {
        {},
        IDX::INDIGO,
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

static constexpr s8 LEVEL_ONE_BLOCK[][10] {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 8, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static constexpr s8 LEVEL0[][10] {
    {4, 5, 6, 1, 2, 3, 4, 5, 6, 7},
    {1, 2, 3, 4, 5, 6, 1, 2, 3, 4},
    {3, 2, 1, 6, 3, 2, 1, 2, 6, 7},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static constexpr s8 LEVEL1[][15] {
    {3, 3, 3, 3, 5, 4, 0, 0, 0, 1, 3, 3, 4, 3, 3},
    {5, 4, 2, 2, 3, 3, 0, 0, 0, 4, 2, 2, 5, 2, 6},
    {3, 4, 3, 2, 2, 6, 0, 0, 0, 2, 2, 5, 3, 2, 5},
    {2, 2, 4, 3, 2, 3, 2, 0, 0, 3, 2, 4, 2, 4, 6},
    {5, 4, 2, 4, 3, 3, 0, 0, 0, 4, 2, 2, 5, 2, 6},
    {3, 1, 1, 1, 2, 6, 0, 0, 0, 2, 2, 5, 3, 2, 5},
    {2, 1, 1, 1, 3, 5, 2, 0, 0, 2, 4, 6, 2, 2, 4},
    {3, 1, 1, 1, 3, 4, 0, 0, 0, 3, 3, 6, 4, 3, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 7, 5, 7, 3},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define LEVEL_INIT(LVL) {.width = sizeof(LVL[0]) / sizeof(LVL[0][0]), .height = sizeof(LVL) / sizeof(LVL[0]), .aTiles = (s8*)LVL}

const inline Level g_lvlOneBlock = LEVEL_INIT(LEVEL_ONE_BLOCK);
const inline Level g_lvl0 = LEVEL_INIT(LEVEL0);
const inline Level g_lvl1 = LEVEL_INIT(LEVEL1);

extern Player g_player;
extern Ball g_ball;
extern Pool<Entity, ASSET_MAX_COUNT> g_aEntities;

} /* namespace game */

namespace adt
{
namespace print
{

inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const game::REFLECT_SIDE x)
{
    ctx.fmt = "{}";
    ctx.fmtIdx = 0;
    constexpr String map[] {
        "NONE", "UP", "RIGHT", "DOWN", "LEFT", "ELAST"
    };

    return printArgs(ctx, map[int(x + 1)]);
}

} /* namespace print */
} /* namespace adt */
