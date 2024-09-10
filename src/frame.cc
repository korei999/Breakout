#include "frame.hh"

#include "Model.hh"
#include "Shader.hh"
#include "Text.hh"
#include "app.hh"
#include "colors.hh"
#include "controls.hh"
#include "game.hh"
#include "math.hh"
#include "parser/Wave.hh"

#include "adt/AllocatorPool.hh"
#include "adt/Arena.hh"
#include "adt/FixedAllocator.hh"
#include "adt/ThreadPool.hh"

using namespace adt;

namespace frame
{

static void mainLoop();

Pair<f32, f32> g_unit; /* draw size unit */

f32 g_fov = 90.0f;
f32 g_uiWidth = 192.0f * 0.75;
f32 g_uiHeight; /* set in prepareDraw */

f64 g_currTime = 0.0;
f64 g_deltaTime = 0.0;
f64 g_lastFrameTime = 0.0;

static Pair<f32, f32> s_aspect(16.0f, 9.0f);

static f64 s_prevTime;
static int s_fpsCount = 0;

static u8 s_aFrameMem[SIZE_8K] {};
static AllocatorPool<Arena, ASSET_MAX_COUNT> s_apAssets;

static Array<game::Entity> s_aEnemies(AllocatorPoolGet(&s_apAssets, SIZE_8K));
static Array<game::Entity*> s_aPEntities(AllocatorPoolGet(&s_apAssets, SIZE_8K));

static Shader s_shBitMap;
static Shader s_shSprite;

static Texture s_tAsciiMap(AllocatorPoolGet(&s_apAssets, SIZE_1M));
static Texture s_tBox(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tBall(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));
static Texture s_tPaddle(AllocatorPoolGet(&s_apAssets, SIZE_1K * 100));

static parser::Wave s_sndBeep(AllocatorPoolGet(&s_apAssets, SIZE_1K * 400));
static parser::Wave s_sndUnatco(AllocatorPoolGet(&s_apAssets, SIZE_1M * 35));

static Text s_textFPS;

static Plain s_plain;

static Ubo s_uboProjView;

game::Player g_player {
    .base {},
    .speed = 0.5,
    .dir {},
};

game::Ball g_ball {
    .base {},
    .bReleased = false,
    .speed = 0.8f,
    .radius = 22.0f,
    .dir {},
};

static void
updateDeltaTime()
{
    g_currTime = utils::timeNowMS();
    g_deltaTime = g_currTime - g_lastFrameTime;
    g_lastFrameTime = g_currTime;
}

static void
loadThings()
{
    g_uiHeight = (g_uiWidth * (f32)app::g_pApp->wHeight) / (f32)app::g_pApp->wWidth;

    s_plain = Plain(GL_STATIC_DRAW);

    g_aAllShaders = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};
    g_aAllTextures = {AllocatorPoolGet(&s_apAssets, SIZE_1K)};

    ShaderLoad(&s_shBitMap, "shaders/font/font.vert", "shaders/font/font.frag");
    ShaderLoad(&s_shSprite, "shaders/2d/sprite.vert", "shaders/2d/sprite.frag");

    ShaderUse(&s_shBitMap);
    ShaderSetI(&s_shBitMap, "tex0", 0);

    ShaderUse(&s_shSprite);
    ShaderSetI(&s_shSprite, "tex0", 0);

    UboCreateBuffer(&s_uboProjView, sizeof(math::M4)*2, GL_DYNAMIC_DRAW);
    UboBindShader(&s_uboProjView, &s_shSprite, "ubProjView", 0);

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

void
run()
{
    WindowSetCursorImage(app::g_pApp, "default");

    s_prevTime = utils::timeNowS();

    WindowBindGlContext(app::g_pApp);
    WindowShowWindow(app::g_pApp);

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl::debugCallback, app::g_pApp);
#endif

    /*glEnable(GL_CULL_FACE);*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    math::V4 gray = {colors::get(colors::IDX::BLACK), 1.0f};
    glClearColor(gray.r, gray.g, gray.b, gray.a);

    loadThings();
    updateDeltaTime(); /* reset delta time before drawing */
    updateDeltaTime();

    /* proc once to get events */
    WindowSwapBuffers(app::g_pApp);
    controls::procMouse();
    controls::procKeys();
    WindowProcEvents(app::g_pApp);

    WindowEnableRelativeMode(app::g_pApp);

    mainLoop();
}

template<typename T>
inline math::V2
nextPos(const T& e, bool bNormalizeDir)
{
    auto dir = bNormalizeDir ? math::normalize(e.dir) : e.dir;
    return e.base.pos + (dir * (g_deltaTime * e.speed));
}

static void
drawFPSCounter(Allocator* pAlloc)
{
    math::M4 proj = math::M4Ortho(0.0f, g_uiWidth, 0.0f, g_uiHeight, -1.0f, 1.0f);
    ShaderUse(&s_shBitMap);
    TextureBind(&s_tAsciiMap, GL_TEXTURE0);
    ShaderSetM4(&s_shBitMap, "uProj", proj);
    ShaderSetV4(&s_shBitMap, "uColor", {colors::hexToV4(0xeeeeeeff), 1.0f});

    f64 currTime = utils::timeNowMS();
    if (currTime >= s_prevTime + 1000.0)
    {
        String s = StringAlloc(pAlloc, s_textFPS.maxSize);
        snprintf(s.pData, s.size, "FPS: %u\nFrame time: %.3f ms", s_fpsCount, g_deltaTime);

        s_fpsCount = 0;
        s_prevTime = currTime;

        TextUpdate(&s_textFPS, pAlloc, s, 0, 0);
    }

    TextDraw(&s_textFPS);
}

static void
drawEntities()
{
    ShaderUse(&s_shSprite);
    GLuint idxLastTex = 0;

    for (auto& e : s_aPEntities)
    {
        if (e->bDead || e->eColor == game::COLOR::INVISIBLE) continue;

        math::M4 tm = math::M4Iden();
        tm = M4Translate(tm, {e->pos.x + e->xOff, e->pos.y + e->yOff, 0.0f});
        tm = M4Scale(tm, {g_unit.x * e->width, g_unit.y * e->height, 1.0f});

        if (idxLastTex != e->texIdx)
        {
            idxLastTex = e->texIdx;
            TextureBind(e->texIdx, GL_TEXTURE0);
        }

        ShaderSetM4(&s_shSprite, "uModel", tm);
        ShaderSetV3(&s_shSprite, "uColor", game::blockColorToV3(e->eColor));
        PlainDraw(&s_plain);
    }
}

enum REFLECT_SIDE : s8 { NONE = -1, UP, RIGHT, DOWN, LEFT, ESIZE };

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

static void
procBlockHit()
{
    bool bAddSound = false;

    for (auto& e : s_aEnemies)
    {
        if (e.bDead || e.eColor == game::COLOR::INVISIBLE) continue;

        math::V2 center = nextPos(g_ball, true);
        math::V2 aabbHalfExtents(g_unit.x / 2 + 4, g_unit.y / 2 + 4);
        math::V2 aabbCenter = e.pos;
        math::V2 diff = center - e.pos;
        math::V2 clamped = math::V2Clamp(diff, -aabbHalfExtents, aabbHalfExtents);
        math::V2 closest = aabbCenter + clamped;

        diff = closest - center;
        auto diffLen = math::V2Length(diff);

        if (diffLen <= g_ball.radius)
        {
            if (e.eColor != game::COLOR::INVISIBLE) e.bDead = true;

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

static void
procPaddleHit()
{
    const auto& bx = g_ball.base.pos.x;
    const auto& by = g_ball.base.pos.y;
    const auto& px = g_player.base.pos.x;
    const auto& py = g_player.base.pos.y;

    if (bx >= px - g_unit.x*2.0 && bx <= px + g_unit.x*2.0 &&
        by >= py - g_unit.y && by <= py - g_unit.y + g_unit.y/2)
    {
        g_ball.base.pos.y = (py - g_unit.y + g_unit.y/2) + 4;
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
    if (g_ball.base.pos.y <= 0.0f - g_unit.y) 
    {
        g_ball.bReleased = false;
    }
    else if (g_ball.base.pos.y >= HEIGHT - g_unit.y)
    {
        g_ball.base.pos.y = HEIGHT - g_unit.y - 4;
        g_ball.dir.y = -1.0f;
        bAddSound = true;
    }
    else if (g_ball.base.pos.x <= 0.0f - g_unit.x)
    {
        g_ball.base.pos.x = -g_unit.x + 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }
    else if (g_ball.base.pos.x >= WIDTH - g_unit.x)
    {
        g_ball.base.pos.x = WIDTH - g_unit.x - 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

static void
updateGame()
{
    /* player */
    {
        g_player.base.pos = nextPos(g_player, false);
        g_player.base.pos = nextPos(g_player, false);

        if (g_player.base.pos.x >= WIDTH - g_unit.x*2)
        {
            g_player.base.pos.x = WIDTH - g_unit.x*2;
            g_player.dir = {};
        }
        else if (g_player.base.pos.x <= 0)
        {
            g_player.base.pos.x = 0;
            g_player.dir = {};
        }

        const auto& pos = g_player.base.pos;
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
        tm = M4Scale(tm, {g_unit.x, g_unit.y, 1.0f});
    }
}

static void
loadLevel()
{
    const auto& level = game::g_level1;

    const int levelY = int(utils::size(level));
    const int levelX = int(sizeof(level) / levelY);

    g_unit.x = WIDTH / levelX / 2;
    g_unit.y = HEIGHT / levelY / 2;

    /* place player in the middle */
    g_player.base.pos.x = WIDTH/2 - g_unit.x;

    for (u32 i = 0; i < levelY; i++)
    {
        for (u32 j = 0; j < levelX; j++)
        {
            if (level[i][j] != s8(game::COLOR::INVISIBLE))
            {
                ArrayPush(&s_aEnemies, {
                    .pos = {g_unit.x*2*j, (HEIGHT - g_unit.y*2) - g_unit.y*2*i},
                    .width = 1.0f,
                    .height = 1.0f,
                    .xOff = 0.0f,
                    .yOff = 0.0f,
                    .shaderIdx = 0,
                    .texIdx = s_tBox.id,
                    .eColor = game::COLOR(level[i][j]),
                    .bDead = false
                });

                ArrayPush(&s_aPEntities, &ArrayLast(&s_aEnemies));
            }
        }
    }

    g_player.base.texIdx = s_tPaddle.id;
    g_player.base.width = 2.0f;
    g_player.base.height = 1.0f;
    g_player.base.xOff = -g_unit.x;
    g_player.base.eColor = game::COLOR::TEAL;

    g_ball.base.eColor = game::COLOR::ORANGERED;
    g_ball.base.texIdx = s_tBall.id;
    g_ball.base.width = 1.0f;
    g_ball.base.height = 1.0f;

    ArrayPush(&s_aPEntities, &g_player.base);
    ArrayPush(&s_aPEntities, &g_ball.base);
}

static void
mainLoop()
{
    loadLevel();

    audio::MixerAddBackground(app::g_pMixer, parser::WaveGetTrack(&s_sndUnatco, true, 0.7f));

    FixedAllocator alFrame (s_aFrameMem, sizeof(s_aFrameMem));

    while (app::g_pApp->bRunning || app::g_pMixer->bRunning) /* wait for mixer to stop also */
    {
        WindowProcEvents(app::g_pApp);
        updateDeltaTime();
        controls::procKeys();

        controls::g_camera.proj = math::M4Ortho(-0.0f, WIDTH, 0.0f, HEIGHT, -50.0f, 50.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app::g_pApp->wWidth, app::g_pApp->wHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        UboBufferData(&s_uboProjView, &controls::g_camera, 0, sizeof(math::M4) * 2);

        updateGame();
        drawEntities();
        drawFPSCounter(&alFrame.base);

        FixedAllocatorReset(&alFrame);
        WindowSwapBuffers(app::g_pApp);
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
