#include "game.hh"

#include "Model.hh"
#include "Shader.hh"
#include "Text.hh"
#include "Texture.hh"
#include "Window.hh"
#include "adt/AllocatorPool.hh"
#include "adt/Arena.hh"
#include "adt/ThreadPool.hh"
#include "app.hh"
#include "audio.hh"
#include "frame.hh"
#include "parser/Wave.hh"

#include <stdio.h>

namespace game
{

static AllocatorPool<Arena, ASSET_MAX_COUNT> s_apAssets;

static Array<Entity> s_aEnemies(AllocatorPoolGet(&s_apAssets, SIZE_8K));

static Shader s_shFontBitMap;
static Shader s_shSprite;

static Texture s_tAsciiMap(AllocatorPoolGet(&s_apAssets, SIZE_1M));
static Texture s_tBox(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tBall(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tPaddle(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));

static parser::Wave s_sndBeep(AllocatorPoolGet(&s_apAssets, SIZE_1K * 400));
static parser::Wave s_sndUnatco(AllocatorPoolGet(&s_apAssets, SIZE_1M * 35));

static Plain s_plain;

static Text s_textFPS;

Array<Entity*> g_aPEntities(AllocatorPoolGet(&s_apAssets, SIZE_8K));

Player g_player {
    .base {},
    .speed = 0.5,
    .dir {},
};

Ball g_ball {
    .base {},
    .bReleased = false,
    .speed = 0.8f,
    .radius = 0.8f,
    .dir {},
};

void
loadThings()
{
    frame::g_uiHeight = (frame::g_uiWidth * (f32)app::g_pApp->wHeight) / (f32)app::g_pApp->wWidth;

    s_plain = Plain(GL_STATIC_DRAW);

    g_aAllShaders = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};
    g_aAllTextures = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};

    ShaderLoad(&s_shFontBitMap, "shaders/font/font.vert", "shaders/font/font.frag");
    ShaderUse(&s_shFontBitMap);
    ShaderSetI(&s_shFontBitMap, "tex0", 0);

    ShaderLoad(&s_shSprite, "shaders/2d/sprite.vert", "shaders/2d/sprite.frag");
    ShaderUse(&s_shSprite);
    ShaderSetI(&s_shSprite, "tex0", 0);
    UboBindShader(&frame::g_uboProjView, &s_shSprite, "ubProjView", 0);

    s_textFPS = Text("", 40, 0, 0, GL_DYNAMIC_DRAW);

    Arena allocScope(SIZE_1K);
    ThreadPool tp(&allocScope.base, 1);
    ThreadPoolStart(&tp);

    /* unbind before creating threads */
    WindowUnbindGlContext(app::g_pApp);

    parser::WaveLoadArg argBeep {&s_sndBeep, "test-assets/c100s16.wav"};
    parser::WaveLoadArg argUnatco {&s_sndUnatco, "test-assets/Unatco.wav"};

    TexLoadArg argFontBitMap {&s_tAsciiMap, "test-assets/bitmapFont20.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST};
    TexLoadArg argBox {&s_tBox, "test-assets/box3.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};
    TexLoadArg argBall {&s_tBall, "test-assets/ball.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};
    TexLoadArg argPaddle {&s_tPaddle, "test-assets/paddle.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};

    ThreadPoolSubmit(&tp, parser::WaveSubmit, &argBeep);
    ThreadPoolSubmit(&tp, parser::WaveSubmit, &argUnatco);

    ThreadPoolSubmit(&tp, TextureSubmit, &argFontBitMap);
    ThreadPoolSubmit(&tp, TextureSubmit, &argBox);
    ThreadPoolSubmit(&tp, TextureSubmit, &argBall);
    ThreadPoolSubmit(&tp, TextureSubmit, &argPaddle);

    ThreadPoolWait(&tp);

    /* restore context after assets are loaded */
    WindowBindGlContext(app::g_pApp);

    ThreadPoolDestroy(&tp);
    ArenaFreeAll(&allocScope);

    WindowSetSwapInterval(app::g_pApp, 1);
    WindowSetFullscreen(app::g_pApp);
}

template<typename T>
inline math::V2
nextPos(const T& e, bool bNormalizeDir)
{
    auto dir = bNormalizeDir ? math::normalize(e.dir) : e.dir;
    return e.base.pos + (dir * (frame::g_deltaTime * e.speed));
}

static REFLECT_SIDE
getReflectionSide(math::V2 tar)
{
    constexpr math::V2 compass[] {
        { 0.0,  1.0 }, /* up */
        { 1.0,  0.0 }, /* right */
        { 0.0, -1.0 }, /* down */
        {-1.0,  0.0 }, /* left */
    };
    f32 max = 0.0f;

    REFLECT_SIDE bestMatch = NONE;
    for (int i = 0; i < REFLECT_SIDE::ESIZE; i++)
    {
        f32 dot = V2Dot(V2Norm(tar), compass[i]);
        if (dot > max)
        {
            max = dot;
            bestMatch = REFLECT_SIDE(i);
        }
    }

    return bestMatch;
}

void
procBlockHit()
{
    bool bAddSound = false;

    for (auto& e : s_aEnemies)
    {
        if (e.bDead || e.eColor == COLOR::INVISIBLE) continue;

        math::V2 center = nextPos(g_ball, true);
        math::V2 aabbHalfExtents(frame::g_unit.x / 2 + 4, frame::g_unit.y / 2 + 4);
        math::V2 aabbCenter = e.pos;
        math::V2 diff = center - e.pos;
        math::V2 clamped = math::V2Clamp(diff, -aabbHalfExtents, aabbHalfExtents);
        math::V2 closest = aabbCenter + clamped;

        diff = closest - center;
        auto diffLen = math::V2Length(diff);

        if (diffLen <= frame::g_unit.x * g_ball.radius)
        {
            if (e.eColor != COLOR::INVISIBLE) e.bDead = true;

            bAddSound = true;

            auto side = getReflectionSide(diff);

            /* FIXME: sides are flipped */
            switch (side)
            {
                default: break;
            
                case REFLECT_SIDE::UP:
                case REFLECT_SIDE::DOWN:
                    g_ball.dir.y = -g_ball.dir.y;
                    break;
            
                case REFLECT_SIDE::LEFT:
                    if (math::rEq(g_ball.dir.x, 0))
                    {
                        g_ball.dir.x = 0.25;
                        break;
                    }
                [[fallthrough]];
                case REFLECT_SIDE::RIGHT:
                    if (math::rEq(g_ball.dir.x, 0))
                    {
                        g_ball.dir.x = -0.25;
                        break;
                    }

                    g_ball.dir.x = -g_ball.dir.x;
                    break;
            }

            break;
        }
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

void
procPaddleHit()
{
    const auto& bx = g_ball.base.pos.x;
    const auto& by = g_ball.base.pos.y;
    const auto& px = g_player.base.pos.x;
    const auto& py = g_player.base.pos.y;

    if (bx >= px - frame::g_unit.x*2.0 && bx <= px + frame::g_unit.x*2.0 &&
        by >= py - frame::g_unit.y && by <= py - frame::g_unit.y + frame::g_unit.y/2)
    {
        g_ball.base.pos.y = (py - frame::g_unit.y + frame::g_unit.y/2) + 4;
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));

        g_ball.dir.y = 1.0f;
        if (math::V2Length(g_player.dir) > 0.0f)
            g_ball.dir += g_player.dir * 0.25f;
    }
}

static void
procOutOfBounds()
{
    /* out of bounds */
    bool bAddSound = false;
    if (g_ball.base.pos.y <= 0.0f - frame::g_unit.y) 
    {
        g_ball.bReleased = false;
    }
    else if (g_ball.base.pos.y >= frame::HEIGHT - frame::g_unit.y)
    {
        g_ball.base.pos.y = frame::HEIGHT - frame::g_unit.y - 4;
        g_ball.dir.y = -1.0f;
        bAddSound = true;
    }
    else if (g_ball.base.pos.x <= 0.0f - frame::g_unit.x)
    {
        g_ball.base.pos.x = -frame::g_unit.x + 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }
    else if (g_ball.base.pos.x >= frame::WIDTH - frame::g_unit.x)
    {
        g_ball.base.pos.x = frame::WIDTH - frame::g_unit.x - 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

void
loadLevel(const Level& lvl)
{
    const auto& l = lvl.aTiles;

    auto at = [&](int y, int x) -> s8& {
        return l[y*lvl.width + x];
    };

    const u32 levelY = lvl.height;
    const u32 levelX = lvl.width;

    frame::g_unit.x = frame::WIDTH / levelX / 2;
    frame::g_unit.y = frame::HEIGHT / levelY / 2;

    /* place player in the middle */
    g_player.base.pos.x = frame::WIDTH/2 - frame::g_unit.x;

    ArraySetSize(&s_aEnemies, 0);
    ArraySetSize(&g_aPEntities, 0);

    for (u32 i = 0; i < levelY; i++)
    {
        for (u32 j = 0; j < levelX; j++)
        {
            if (at(i, j) != s8(COLOR::INVISIBLE))
            {
                ArrayPush(&s_aEnemies, {
                    .pos = {frame::g_unit.x*2*j, (frame::HEIGHT - frame::g_unit.y*2) - frame::g_unit.y*2*i},
                    .width = 1.0f,
                    .height = 1.0f,
                    .xOff = 0.0f,
                    .yOff = 0.0f,
                    .shaderIdx = 0,
                    .texIdx = s_tBox.id,
                    .eColor = COLOR(at(i, j)),
                    .bDead = false
                });

                ArrayPush(&g_aPEntities, &ArrayLast(&s_aEnemies));
            }
        }
    }

    g_player.base.texIdx = s_tPaddle.id;
    g_player.base.width = 2.0f;
    g_player.base.height = 1.0f;
    g_player.base.xOff = -frame::g_unit.x;
    g_player.base.eColor = COLOR::TEAL;

    g_ball.base.eColor = COLOR::ORANGERED;
    g_ball.base.texIdx = s_tBall.id;
    g_ball.base.width = 1.0f;
    g_ball.base.height = 1.0f;

    ArrayPush(&g_aPEntities, &g_player.base);
    ArrayPush(&g_aPEntities, &g_ball.base);

    audio::MixerAddBackground(app::g_pMixer, parser::WaveGetTrack(&s_sndUnatco, true, 0.7f));
}

void
updateGame()
{
    /* player */
    {
        g_player.base.pos = nextPos(g_player, false);
        g_player.base.pos = nextPos(g_player, false);

        if (g_player.base.pos.x >= frame::WIDTH - frame::g_unit.x*2)
        {
            g_player.base.pos.x = frame::WIDTH - frame::g_unit.x*2;
            g_player.dir = {};
        }
        else if (g_player.base.pos.x <= 0)
        {
            g_player.base.pos.x = 0;
            g_player.dir = {};
        }
    }

    /* ball */
    {
        if (g_ball.bReleased)
        {
            procBlockHit();
            procPaddleHit();
            procOutOfBounds();
            g_ball.base.pos = nextPos(g_ball, true);

        } else g_ball.base.pos = g_player.base.pos;

        const auto& pos = g_ball.base.pos;

        math::M4 tm;
        tm = math::M4Iden();
        tm = M4Translate(tm, {pos.x, pos.y, 10.0f});
        tm = M4Scale(tm, {frame::g_unit.x, frame::g_unit.y, 1.0f});
    }
}

void
drawFPSCounter(Allocator* pAlloc)
{
    math::M4 proj = math::M4Ortho(0.0f, frame::g_uiWidth, 0.0f, frame::g_uiHeight, -1.0f, 1.0f);
    ShaderUse(&s_shFontBitMap);
    TextureBind(&s_tAsciiMap, GL_TEXTURE0);
    ShaderSetM4(&s_shFontBitMap, "uProj", proj);
    ShaderSetV4(&s_shFontBitMap, "uColor", {colors::hexToV4(0xeeeeeeff), 1.0f});

    f64 currTime = utils::timeNowMS();
    if (currTime >= frame::g_prevTime + 1000.0)
    {
        String s = StringAlloc(pAlloc, s_textFPS.maxSize);
        snprintf(s.pData, s.size, "FPS: %u\nFrame time: %.3f ms", frame::g_fpsCount, frame::g_deltaTime);

        frame::g_fpsCount = 0;
        frame::g_prevTime = currTime;

        TextUpdate(&s_textFPS, pAlloc, s, 0, 0);
    }

    TextDraw(&s_textFPS);
}

void
drawEntities()
{
    ShaderUse(&s_shSprite);
    GLuint idxLastTex = 0;

    for (auto& e : g_aPEntities)
    {
        if (e->bDead || e->eColor == COLOR::INVISIBLE) continue;

        math::M4 tm = math::M4Iden();
        tm = M4Translate(tm, {e->pos.x + e->xOff, e->pos.y + e->yOff, 0.0f});
        tm = M4Scale(tm, {frame::g_unit.x * e->width, frame::g_unit.y * e->height, 1.0f});

        if (idxLastTex != e->texIdx)
        {
            idxLastTex = e->texIdx;
            TextureBind(e->texIdx, GL_TEXTURE0);
        }

        ShaderSetM4(&s_shSprite, "uModel", tm);
        ShaderSetV3(&s_shSprite, "uColor", blockColorToV3(e->eColor));
        PlainDraw(&s_plain);
    }
}

void
cleanup()
{
    PlainDestroy(&s_plain);

    for (auto& e : g_aAllShaders)
        ShaderDestroy(&e);

    for (auto& t : g_aAllTextures)
        TextureDestroy(&t);

    for (auto& a : s_apAssets.aAllocators)
        ArenaFreeAll(&a);
}

} /* namespace game */
