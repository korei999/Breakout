#include <stdio.h>

#include "adt/AllocatorPool.hh"
#include "adt/Arena.hh"
#include "adt/ThreadPool.hh"
#include "adt/utils.hh"
#include "adt/FixedAllocator.hh"

#include "Model.hh"
#include "Shader.hh"
#include "Text.hh"
#include "colors.hh"
#include "frame.hh"
#include "math.hh"
#include "parser/Wave.hh"

using namespace adt;

namespace frame
{

static void mainLoop();

App* g_pApp;
audio::Mixer* g_pMixer;

controls::Player g_player {
    .moveSpeed = 0.9,
    .dir {},
    .mouse {.sens = 0.07},
};

game::Ball g_ball {
    .bReleased = false,
    .speed = 0.8f,
    .radius = 22.0f,
    .pos = g_player.pos,
    .dir {},
};

Pair<f32, f32> g_unit;

f32 g_fov = 90.0f;
f32 g_uiWidth = 192.0f;
f32 g_uiHeight; /* set in prepareDraw */

f64 g_currTime = 0.0;
f64 g_deltaTime = 0.0;
f64 g_lastFrameTime = 0.0;

static Pair<f32, f32> s_aspect(16.0f, 9.0f);

static f64 s_prevTime;
static int s_fpsCount = 0;
static char s_fpsStrBuff[40] {};

static AllocatorPool<Arena, ASSET_MAX_COUNT> s_apAssets;

static Array<game::Entity> s_enemies(AllocatorPoolGet(&s_apAssets, SIZE_8K));

static Shader s_shTex;
static Shader s_shBitMap;
static Shader s_shColor;
static Shader s_shSprite;

static Texture s_tAsciiMap(AllocatorPoolGet(&s_apAssets, SIZE_1M));
static Texture s_tSampleTex(AllocatorPoolGet(&s_apAssets, SIZE_1M));
static Texture s_tBox(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tBall(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tPaddle(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));

static parser::Wave s_sndBeep(AllocatorPoolGet(&s_apAssets, SIZE_1K * 400));
static parser::Wave s_sndDuClare(AllocatorPoolGet(&s_apAssets, SIZE_1M * 35));
static parser::Wave s_sndUnatco(AllocatorPoolGet(&s_apAssets, SIZE_1M * 35));

static Text s_textFPS;
static Text s_textTest;

static Plain s_plain;

static Ubo s_uboProjView;

static void
updateDeltaTime()
{
    g_currTime = utils::timeNowMS();
    g_deltaTime = g_currTime - g_lastFrameTime;
    g_lastFrameTime = g_currTime;
}

void
prepareDraw()
{
    AppBindGlContext(g_pApp);
    AppShowWindow(g_pApp);

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, g_pApp);
#endif

    /*glEnable(GL_CULL_FACE);*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    math::V4 gray = {colors::black, 1.0f};
    glClearColor(gray.r, gray.g, gray.b, gray.a);

    g_uiHeight = (g_uiWidth * (f32)g_pApp->wHeight) / (f32)g_pApp->wWidth;

    s_plain = Plain(GL_STATIC_DRAW);

    g_aAllShaders = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};
    g_aAllTextures = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};

    ShaderLoad(&s_shTex, "shaders/simpleTex.vert", "shaders/simpleTex.frag");
    ShaderLoad(&s_shColor, "shaders/simpleUB.vert", "shaders/simple.frag");
    ShaderLoad(&s_shBitMap, "shaders/font/font.vert", "shaders/font/font.frag");
    ShaderLoad(&s_shSprite, "shaders/2d/sprite.vert", "shaders/2d/sprite.frag");

    ShaderUse(&s_shTex);
    ShaderSetI(&s_shTex, "tex0", 0);

    ShaderUse(&s_shBitMap);
    ShaderSetI(&s_shBitMap, "tex0", 0);

    ShaderUse(&s_shSprite);
    ShaderSetI(&s_shSprite, "tex0", 0);

    UboCreateBuffer(&s_uboProjView, sizeof(math::M4)*2, GL_DYNAMIC_DRAW);
    UboBindShader(&s_uboProjView, &s_shTex, "ubProjView", 0);
    UboBindShader(&s_uboProjView, &s_shColor, "ubProjView", 0);

    s_textFPS = Text("", utils::size(s_fpsStrBuff), 0, 0, GL_DYNAMIC_DRAW);
    s_textTest = Text("", 256, 0, 0, GL_DYNAMIC_DRAW);

    Arena allocScope(SIZE_1K);
    ThreadPool tp(&allocScope.base, 1);
    ThreadPoolStart(&tp);

    /* unbind before creating threads */
    AppUnbindGlContext(g_pApp);

    parser::WaveLoadArg argBeep {&s_sndBeep, "test-assets/c100s16.wav"};
    parser::WaveLoadArg argUnatco {&s_sndUnatco, "test-assets/Unatco.wav"};

    TexLoadArg argFontBitMap {&s_tAsciiMap, "test-assets/bitmapFont2.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST};
    TexLoadArg argSampleTex {&s_tSampleTex, "test-assets/dirt.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST};
    TexLoadArg argBox {&s_tBox, "test-assets/box3.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};
    TexLoadArg argBall {&s_tBall, "test-assets/ball.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};
    TexLoadArg argPaddle {&s_tPaddle, "test-assets/paddle.bmp", TEX_TYPE::DIFFUSE, false, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST};

    ThreadPoolSubmit(&tp, parser::WaveSubmit, &argBeep);
    ThreadPoolSubmit(&tp, parser::WaveSubmit, &argUnatco);

    ThreadPoolSubmit(&tp, TextureSubmit, &argSampleTex);
    ThreadPoolSubmit(&tp, TextureSubmit, &argFontBitMap);
    ThreadPoolSubmit(&tp, TextureSubmit, &argBox);
    ThreadPoolSubmit(&tp, TextureSubmit, &argBall);
    ThreadPoolSubmit(&tp, TextureSubmit, &argPaddle);

    ThreadPoolWait(&tp);

    /* restore context after assets are loaded */
    AppBindGlContext(g_pApp);

    ThreadPoolDestroy(&tp);
    ArenaFreeAll(&allocScope);

    AppSetSwapInterval(g_pApp, 1);
    AppSetFullscreen(g_pApp);
}

void
run()
{
    g_pApp->bRunning = true;
    AppSetCursorImage(g_pApp, "default");

    s_prevTime = utils::timeNowS();

    prepareDraw();
    updateDeltaTime(); /* reset delta time before drawing */
    updateDeltaTime();

    /* proc once to get events */
    AppSwapBuffers(g_pApp);
    controls::PlayerProcMouse(&g_player);
    controls::PlayerProcKeys(&g_player);
    AppProcEvents(g_pApp);

    if (!g_pApp->bPointerRelativeMode)
        AppEnableRelativeMode(g_pApp);

    mainLoop();
}

static void
renderFPSCounter(Allocator* pAlloc)
{
    math::M4 proj = math::M4Ortho(0.0f, g_uiWidth, 0.0f, g_uiHeight, -1.0f, 1.0f);
    ShaderUse(&s_shBitMap);
    TextureBind(&s_tAsciiMap, GL_TEXTURE0);
    ShaderSetM4(&s_shBitMap, "uProj", proj);

    f64 currTime = utils::timeNowMS();
    if (currTime >= s_prevTime + 1000.0)
    {
        memset(s_fpsStrBuff, 0, utils::size(s_fpsStrBuff));
        snprintf(s_fpsStrBuff, utils::size(s_fpsStrBuff), "FPS: %u\nFrame time: %.3f ms", s_fpsCount, g_deltaTime);

        s_fpsCount = 0;
        s_prevTime = currTime;

        TextUpdate(&s_textFPS, pAlloc, s_fpsStrBuff, 0, 0);
    }

    TextDraw(&s_textFPS);
}

enum REFLECT_SIDE : s8 { NONE = -1, UP, RIGHT, DOWN, LEFT, ESIZE };

inline REFLECT_SIDE
getReflectionSide(math::V2 tar)
{
    constexpr math::V2 compass[] {
        { 0.0,  1.0 }, /* up */
        { 1.0,  0.0 }, /* right */
        { 0.0, -1.0 }, /* down */
        {-1.0, -1.0 }, /* left */
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

inline void
procBlockHit()
{
    bool bAddSound = false;

    for (auto& e : s_enemies)
    {
        if (e.bDead) continue;

        math::V2 center = g_ball.pos;
        math::V2 aabbHalfExtents(g_unit.x / 2, g_unit.y / 2);
        math::V2 aabbCenter = e.pos;
        math::V2 diff = center - e.pos;
        math::V2 clamped = math::V2Clamp(diff, -aabbHalfExtents, aabbHalfExtents);
        math::V2 closest = aabbCenter + clamped;

        diff = closest - center;
        auto diffLen = math::V2Length(diff);

        if (diffLen < g_ball.radius)
        {
            auto side = getReflectionSide(diff);

            /* FIXME: sides are flipped */
            if (side == REFLECT_SIDE::UP)
            {
                /*COUT("UP\n");*/
                f32 penetration = g_ball.radius - fabs(diff.y);
                g_ball.pos.y -= (penetration + 10);
                g_ball.dir.y = -g_ball.dir.y;
            }
            else if (side == REFLECT_SIDE::DOWN)
            {
                /*COUT("DOWN\n");*/
                f32 penetration = g_ball.radius - fabs(diff.y);
                g_ball.pos.y += (penetration + 10);
                g_ball.dir.y = -g_ball.dir.y;
            }
            else if (side == REFLECT_SIDE::LEFT)
            {
                /*COUT("LEFT\n");*/
                /*f32 penetration = g_ball.radius - fabs(diff.y);*/
                /*g_ball.pos.x += (penetration);*/
                g_ball.dir.x = -g_ball.dir.x;
            }
            else if (side == REFLECT_SIDE::RIGHT)
            {
                /*COUT("RIGHT\n");*/
                /*f32 penetration = g_ball.radius - fabs(diff.y);*/
                /*g_ball.pos.x -= (penetration);*/
                g_ball.dir.x = -g_ball.dir.x;
            }

            if (e.color != game::BLOCK_COLOR::GRAY) e.bDead = true;

            bAddSound = true;
        }
    }

    if (bAddSound)
        audio::MixerAdd(g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

inline void
procPaddleHit()
{
    const auto& bx = g_ball.pos.x;
    const auto& by = g_ball.pos.y;
    const auto& px = g_player.pos.x;
    const auto& py = g_player.pos.y;

    if (bx >= px - g_unit.x*2.0 && bx <= px + g_unit.x*2.0 &&
        by >= py - g_unit.y && by <= py - g_unit.y + g_unit.y/2)
    {
        g_ball.pos.y = (py - g_unit.y + g_unit.y/2) + 4;
        audio::MixerAdd(g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));

        g_ball.dir.y = 1.0f;
        if (math::V2Length(g_player.dir) > 0.0f)
            g_ball.dir += g_player.dir * 0.25f;
    }
}

inline void
procOutOfBounds()
{
    /* out of bounds */
    bool bAddSound = false;
    if (g_ball.pos.y <= 0.0f - g_unit.y) 
    {
        g_ball.bReleased = false;
        /*g_ball.pos.y = 0.0f - g_unit.y + 4;*/
        /*g_ball.dir.y = -g_ball.dir.y;*/
        /*bAddSound = true;*/
    }
    else if (g_ball.pos.y >= HEIGHT - g_unit.y)
    {
        g_ball.pos.y = HEIGHT - g_unit.y - 4;
        g_ball.dir.y = -1.0f;
        bAddSound = true;
    }
    else if (g_ball.pos.x <= 0.0f - g_unit.x)
    {
        g_ball.pos.x = -g_unit.x + 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }
    else if (g_ball.pos.x >= WIDTH - g_unit.x)
    {
        g_ball.pos.x = WIDTH - g_unit.x - 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }

    if (bAddSound)
        audio::MixerAdd( g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

static void
mainLoop()
{
    auto& level = game::g_level1;

    constexpr int levelY = (int)utils::size(level);
    constexpr int levelX = (int)sizeof(level) / levelY;

    g_unit.x = WIDTH / levelX / 2;
    g_unit.y = HEIGHT / levelY / 2;

    for (u32 i = 0; i < levelY; i++)
    {
        for (u32 j = 0; j < levelX; j++)
        {
            if (level[i][j] != s8(game::BLOCK_COLOR::INVISIBLE))
            {
                ArrayPush(&s_enemies, {
                    .pos = {g_unit.x*2*j, (HEIGHT - g_unit.y*2) - g_unit.y*2*i},
                    .shaderIdx = 0,
                    .modelIdx = 0,
                    .texIdx = 0,
                    .color = (game::BLOCK_COLOR)level[i][j],
                    .bDead = false
                });
            }
        }
    }

    audio::MixerAddBackground(g_pMixer, parser::WaveGetTrack(&s_sndUnatco, true, 0.8f));

    while (g_pApp->bRunning || g_pMixer->bRunning) /* wait for mixer to stop also */
    {
        u8 aFrameMem[SIZE_8K] {};
        FixedAllocator allocFrame (aFrameMem, sizeof(aFrameMem));

        {
            AppProcEvents(g_pApp);
            updateDeltaTime();
            PlayerProcKeys(&g_player);

            g_player.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, g_pApp->wWidth, g_pApp->wHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            UboBufferData(&s_uboProjView, &g_player, 0, sizeof(math::M4) * 2);

            /* enemies */
            TextureBind(&s_tBox, GL_TEXTURE0);
            ShaderUse(&s_shSprite);
            for (auto& enemy : s_enemies)
            {
                if (enemy.bDead) continue;

                /*math::V3 pos {((sinf(i) * WIDTH) + WIDTH) / 2.0f, enemy.pos.y, 0.0f};*/
                math::V3 pos {enemy.pos.x, enemy.pos.y, 0.0f};

                /*enemy.pos = pos;*/

                math::M4 tm;
                tm = math::M4Iden();
                tm = M4Translate(tm, pos);
                tm = M4Scale(tm, {g_unit.x, g_unit.y, 1.0f});

                ShaderSetM4(&s_shSprite, "uModel", tm);
                ShaderSetV3(&s_shSprite, "uColor", game::blockColorToV3(enemy.color));
                PlainDraw(&s_plain);

                /*TextureBind(&s_tSampleTex, GL_TEXTURE0);*/
                /*PlainDrawBox(&s_plain);*/
            }

            /* player */
            {
                g_player.pos += g_player.dir * g_player.moveSpeed * g_deltaTime;

                if (g_player.pos.x >= WIDTH - g_unit.x*2)
                {
                    g_player.pos.x = WIDTH - g_unit.x*2;
                    g_player.dir = {};
                }
                else if (g_player.pos.x <= 0)
                {
                    g_player.pos.x = 0;
                    g_player.dir = {};
                }

                const auto& pos = g_player.pos;

                math::M4 tm;
                tm = math::M4Iden();
                tm = M4Translate(tm, {pos.x - g_unit.x, pos.y, 10.0f});

                tm = M4Scale(tm, {g_unit.x*2, g_unit.y, 1.0f});

                ShaderUse(&s_shSprite);
                TextureBind(&s_tPaddle, GL_TEXTURE0);
                ShaderSetM4(&s_shSprite, "uModel", tm);
                ShaderSetV3(&s_shSprite, "uColor", colors::teal);
                PlainDraw(&s_plain);
            }

            /* ball */
            {
                if (g_ball.bReleased)
                {
                    g_ball.pos += V2Norm(g_ball.dir) * g_deltaTime * g_ball.speed;

                    procBlockHit();
                    procPaddleHit();
                    procOutOfBounds();
                } else g_ball.pos = g_player.pos;

                const auto& pos = g_ball.pos;

                math::M4 tm;
                tm = math::M4Iden();
                tm = M4Translate(tm, {pos.x, pos.y, 10.0f});
                tm = M4Scale(tm, {g_unit.x, g_unit.y, 1.0f});

                ShaderUse(&s_shSprite);
                TextureBind(&s_tBall, GL_TEXTURE0);
                ShaderSetM4(&s_shSprite, "uModel", tm);
                ShaderSetV3(&s_shSprite, "uColor", colors::tomato);
                PlainDraw(&s_plain);
            }

            renderFPSCounter(&allocFrame.base);
        }

        AppSwapBuffers(g_pApp);

        s_fpsCount++;
    }

#ifndef NDEBUG

    PlainDestroy(&s_plain);
    UboDestroy(&s_uboProjView);

    for (auto& e : g_aAllShaders)
        ShaderDestroy(&e);

    for (auto& t : g_aAllTextures)
        TextureDestroy(&t);

    for (auto& a : s_apAssets.aAllocators)
        ArenaFreeAll(&a);

#endif
}

} /* namespace frame */
