#include "game.hh"

#include "IWindow.hh"
#include "Shader.hh"
#include "adt/AllocatorPool.hh"
#include "adt/Arena.hh"
#include "adt/Pool.hh"
#include "adt/ThreadPool.hh"
#include "adt/defer.hh"
#include "app.hh"
#include "frame.hh"
#include "parser/Wave.hh"
#include "parser/ttf.hh"
#include "text.hh"
#include "texture.hh"

namespace game
{

template<typename A, typename B>
struct WidthHeight
{
    A width {};
    B height {};
};

static AllocatorPool<Arena> s_assetArenas(SIZE_MIN);

static Vec<game::Block> s_aBlocks(AllocatorPoolRent(&s_assetArenas, SIZE_1K));

static Shader s_shFontBitmap;
static Shader s_shSprite;
static Shader s_sh1Col;

static texture::Img s_tAsciiMap(AllocatorPoolRent(&s_assetArenas, SIZE_1M));
static texture::Img s_tBox(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tBall(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tPaddle(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tWhitePixel(AllocatorPoolRent(&s_assetArenas, 250));

static parser::Wave s_sndBeep(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 400));
static parser::Wave s_sndUnatco(AllocatorPoolRent(&s_assetArenas, SIZE_1M * 35));

static Plain s_plain;

static parser::ttf::Font s_fontLiberation(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 500));
static text::Bitmap s_textFPS;
static text::TTF s_ttfTest(AllocatorPoolRent(&s_assetArenas, SIZE_1K * 520));

Pool<Entity, ASSET_MAX_COUNT> g_aEntities;
static Arr<math::V2, ASSET_MAX_COUNT> s_aPrevPos;
static Pair<f32, f32, WidthHeight> s_currLvlSize {};

Player g_player {
    .enIdx = 0,
};

Ball g_ball {
    .enIdx = 0,
    .radius = 0.1f,
    .bReleased = false,
    .bCollided = false,
};

static void drawFPSCounter(Arena* pAlloc);
static void drawFPSCounterTTF(Arena* pAlloc);
static void drawInfo(Arena* pArena);
static void drawEntities(Arena* pAlloc, const f64 alpha);
static void drawTTF(Arena* pAlloc);

void
loadAssets()
{
    frame::g_uiHeight = (frame::g_uiWidth * (f32)app::g_pWindow->wHeight) / (f32)app::g_pWindow->wWidth;

    s_plain = Plain(GL_STATIC_DRAW);

    ShaderLoad(&s_shFontBitmap, "shaders/font/font.vert", "shaders/font/font.frag");
    ShaderUse(&s_shFontBitmap);
    ShaderSetI(&s_shFontBitmap, "tex0", 0);

    ShaderLoad(&s_sh1Col, "shaders/font/1col.vert", "shaders/font/1col.frag");
    ShaderUse(&s_sh1Col);
    ShaderSetI(&s_sh1Col, "uTex0", 0);

    ShaderLoad(&s_shSprite, "shaders/2d/sprite.vert", "shaders/2d/sprite.frag");
    ShaderUse(&s_shSprite);
    ShaderSetI(&s_shSprite, "tex0", 0);

    UboBindShader(&frame::g_uboProjView, &s_shSprite, "ubProjView", 0);

    s_textFPS = text::Bitmap("", 40, 0, 0, GL_DYNAMIC_DRAW);
    parser::ttf::FontLoadParse(&s_fontLiberation, "test-assets/LiberationMono-Regular.ttf");

    /* unbind before creating threads */
    WindowUnbindGlContext(app::g_pWindow);
    defer( WindowBindGlContext(app::g_pWindow) );

    text::TTFRasterizeArg argTTF {&s_ttfTest, &s_fontLiberation};

    parser::WaveLoadArg argBeep {&s_sndBeep, "test-assets/c100s16.wav"};
    parser::WaveLoadArg argUnatco {&s_sndUnatco, "test-assets/Unatco.wav"};

    texture::ImgLoadArg argFontBitmap {&s_tAsciiMap, "test-assets/bitmapFont20.bmp"};
    texture::ImgLoadArg argBox {&s_tBox, "test-assets/box3.bmp"};
    texture::ImgLoadArg argBall {&s_tBall, "test-assets/ball.bmp"};
    texture::ImgLoadArg argPaddle {&s_tPaddle, "test-assets/paddle.bmp"};
    texture::ImgLoadArg argWhitePixel {&s_tWhitePixel, "test-assets/WhitePixel.bmp"};

    ThreadPoolSubmit(app::g_pThreadPool, text::TTFRasterizeSubmit, &argTTF);

    ThreadPoolSubmit(app::g_pThreadPool, parser::WaveSubmit, &argBeep);
    ThreadPoolSubmit(app::g_pThreadPool, parser::WaveSubmit, &argUnatco);

    ThreadPoolSubmit(app::g_pThreadPool, texture::ImgSubmit, &argFontBitmap);
    ThreadPoolSubmit(app::g_pThreadPool, texture::ImgSubmit, &argBox);
    ThreadPoolSubmit(app::g_pThreadPool, texture::ImgSubmit, &argBall);
    ThreadPoolSubmit(app::g_pThreadPool, texture::ImgSubmit, &argPaddle);
    ThreadPoolSubmit(app::g_pThreadPool, texture::ImgSubmit, &argWhitePixel);

    ThreadPoolWait(app::g_pThreadPool);
}

template<typename T>
static inline math::V2
nextPos(const T& e, bool bNormalizeDir)
{
    auto dir = bNormalizeDir ? math::normalize(e.dir) : e.dir;
    return e.pos + (dir * (frame::g_dt * e.speed));
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

    REFLECT_SIDE eBestMatch = NONE;
    for (u64 i = 0; i < utils::size(compass); i++)
    {
        f32 dot = V2Dot(V2Norm(tar), compass[i]);
        if (dot >= max)
        {
            max = dot;
            eBestMatch = REFLECT_SIDE(i);
        }
    }

    return eBestMatch;
}

static void
blockHit()
{
    bool bAddSound = false;

    auto& enBall = g_aEntities[g_ball.enIdx];
    for (auto& block : s_aBlocks)
    {
        auto& b = g_aEntities[block.enIdx];

        if (b.bDead || b.eColor == COLOR::INVISIBLE) continue;

        math::V2 center = nextPos(enBall, true);
        math::V2 aabbHalfExtents {b.width/2.0f, b.height/2.0f};
        math::V2 aabbCenter = b.pos;
        math::V2 diff = center - b.pos;
        math::V2 clamped = math::V2Clamp(diff, -aabbHalfExtents, aabbHalfExtents);
        math::V2 closest = aabbCenter + clamped;
        diff = closest - center;
        auto diffLen = math::V2Length(diff);

        const auto& bx = enBall.pos.x;
        const auto& by = enBall.pos.y;

        const auto& ex = b.pos.x;
        const auto& ey = b.pos.y;

        if (diffLen <= g_ball.radius)
        {
            if (g_ball.bCollided)
            {
                g_ball.bCollided = false;
                break;
            }

            if (b.eColor != COLOR::INVISIBLE && b.eColor != COLOR::DIMGRAY)
                b.bDead = true;

            bAddSound = true;
            g_ball.bCollided = true;

            auto eSide = getReflectionSide(diff);

            /* FIXME: sides are flipped */
            auto& enBall = g_aEntities[g_ball.enIdx];
            const f32 off = 0.05f;
            switch (eSide)
            {
                default: break;

                case REFLECT_SIDE::UP:
                {
                    enBall.pos.y -= off;
                    enBall.dir.y = -enBall.dir.y;
                    LOG("UP\n");
                } break;

                case REFLECT_SIDE::DOWN:
                {
                    enBall.pos.y += off;
                    enBall.dir.y = -enBall.dir.y;
                    LOG("DOWN\n");
                } break;

                case REFLECT_SIDE::LEFT:
                {
                    enBall.pos.x += off;
                    if (math::eq(enBall.dir.x, 0))
                    {
                        enBall.dir.x = 0.1f;
                        break;
                    }

                    enBall.dir.x = -enBall.dir.x;
                    LOG("LEFT\n");
                } break;

                case REFLECT_SIDE::RIGHT:
                {
                    enBall.pos.x -= off;
                    if (math::eq(enBall.dir.x, 0))
                    {
                        enBall.dir.x = -0.1f;
                        break;
                    }

                    enBall.dir.x = -enBall.dir.x;
                    LOG("RIGHT\n");
                } break;
            }
        }
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.0f));
}

static void
paddleHit()
{
    auto& enBall = g_aEntities[g_ball.enIdx];
    auto& enPlayer = g_aEntities[g_player.enIdx];

    const auto& bx = enBall.pos.x;
    const auto& by = enBall.pos.y;
    const auto& px = enPlayer.pos.x;
    const auto& py = enPlayer.pos.y;

    const f32 pxOff = enPlayer.width / 2.0f;
    const f32 pyOff = enPlayer.height / 2.0f;

    bool bLR = (bx >= px - pxOff && bx <= px + pxOff);
    bool bTB = (by <= py - pyOff/2.0f);

    if (bLR && bTB)
    {
        enBall.pos.y = py - pyOff/2.0f;

        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.0f));

        enBall.dir.y = 1.0f;
        if (math::V2Length(enPlayer.dir) > 0.0f)
            enBall.dir += enPlayer.dir * 0.1f;
    }
}

static void
outOfBounds()
{
    auto& enBall = g_aEntities[g_ball.enIdx];

    /* out of bounds */
    bool bAddSound = false;
    if (enBall.pos.y <= -0.5f) 
    {
        g_ball.bReleased = false;
        // g_ball.base.pos.y = 0.0f - f::g_unit.second;
        // g_ball.dir.y = 1.0f;
        // bAddSound = true;
    }
    else if (enBall.pos.y >= s_currLvlSize.height - 0.5f)
    {
        enBall.pos.y = s_currLvlSize.height - 0.55f;
        enBall.dir.y = -1.0f;
        bAddSound = true;
    }
    else if (enBall.pos.x <= -0.5f)
    {
        enBall.pos.x = -0.45f;
        enBall.dir.x = -enBall.dir.x;
        bAddSound = true;
    }
    else if (enBall.pos.x >= s_currLvlSize.width - 0.5f)
    {
        enBall.pos.x = s_currLvlSize.width - 0.55f;
        enBall.dir.x = -enBall.dir.x;
        bAddSound = true;
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

static inline math::V2
tilePosToImagePos(f32 x, f32 y)
{
    namespace f = frame;
    return {
        .x = f::g_unit.first*2 * x,
        .y = f::g_unit.second*2 * y,
    };
}

void
loadLevel()
{
    const auto& lvl = g_lvl1;
    const auto& l = lvl.aTiles;

    auto at = [&](int y, int x) -> s8& {
        return l[y*lvl.width + x];
    };

    frame::g_unit.first = frame::WIDTH / lvl.width / 2;
    frame::g_unit.second = frame::HEIGHT / lvl.height / 2;

    VecSetCap(&s_aBlocks, lvl.height*lvl.width);
    VecSetSize(&s_aBlocks, 0);

    auto fBoxTex = MapSearch(&texture::g_mAllTexturesIdxs, {"test-assets/box3.bmp"});
    auto boxTexId = texture::g_aAllTextures[fBoxTex.pData->val].id;

    LOG_NOTIFY("width: {}, height: {}\n", lvl.width, lvl.height);
    for (u32 y = 0; y < lvl.height; ++y)
    {
        for (u32 x = 0; x < lvl.width; ++x)
        {
            if (at(y, x) != s8(COLOR::INVISIBLE))
            {
                u32 idx = PoolRent(&g_aEntities);
                auto& e = g_aEntities[idx];

                VecPush(&s_aBlocks, {u16(idx)});
                e.pos = {x, lvl.height - y - 1};
                e.width = 1.0f;
                e.height = 1.0f;
                e.xOff = 0.0f;
                e.yOff = 0.0f;
                e.zOff = 0.0f;
                e.shaderIdx = 0;
                e.texIdx = u16(boxTexId);
                e.eColor = COLOR(at(y, x));
                e.bDead = false;
                e.bRemoveAfterDraw = false;
            }
        }
    }

    g_player.enIdx = PoolRent(&g_aEntities);
    auto& enPlayer = g_aEntities[g_player.enIdx];
    enPlayer.speed = 10.0f;
    enPlayer.pos.x = lvl.width / 2.0f;
    enPlayer.texIdx = s_tPaddle.id;
    enPlayer.width = 2.0f;
    enPlayer.height = 1.0f;
    enPlayer.xOff = -0.5f;
    enPlayer.zOff = 10.0f;
    enPlayer.eColor = COLOR::TEAL;
    enPlayer.bRemoveAfterDraw = false;

    g_ball.enIdx = PoolRent(&g_aEntities);
    auto& enBall = g_aEntities[g_ball.enIdx];
    enBall.speed = 10.0f;
    enBall.eColor = COLOR::ORANGERED;
    enBall.texIdx = s_tBall.id;
    enBall.width = 1.0f;
    enBall.height = 1.0f;
    enBall.zOff = 10.0f;
    enBall.bRemoveAfterDraw = false;

    audio::MixerAddBackground(app::g_pMixer, parser::WaveGetTrack(&s_sndUnatco, true, 0.7f));

    ArrSetSize(&s_aPrevPos, 0);
    for (auto& en : g_aEntities)
        ArrPush(&s_aPrevPos, en.pos);

    s_currLvlSize.width = lvl.width;
    s_currLvlSize.height = lvl.height;
}

void
updateState()
{
    auto& enBall = g_aEntities[g_ball.enIdx];
    auto& enPlayer = g_aEntities[g_player.enIdx];

    /* keep prev positions */
    {
        for (auto& en : g_aEntities)
        {
            auto idx = PoolIdx(&g_aEntities, &en);
            s_aPrevPos[idx] = en.pos;
        }
    }

    /* player */
    {
        enPlayer.pos = nextPos(enPlayer, false);

        if (enPlayer.pos.x >= s_currLvlSize.width - 1.0f)
        {
            enPlayer.pos.x = s_currLvlSize.width - 1.0f;
            enPlayer.dir = {};
        }
        else if (enPlayer.pos.x <= 0)
        {
            enPlayer.pos.x = 0;
            enPlayer.dir = {};
        }
    }

    /* ball */
    {
        if (g_ball.bReleased)
        {
            blockHit();
            paddleHit();
            outOfBounds();
            enBall.pos = nextPos(enBall, true);

        } else enBall.pos = enPlayer.pos;
    }
}

void
draw(Arena* pArena, const f64 alpha)
{
    drawEntities(pArena, alpha);

    drawFPSCounterTTF(pArena);
    drawInfo(pArena);
}

static void
drawFPSCounter(Arena* pAlloc)
{
    math::M4 proj = math::M4Ortho(0.0f, frame::g_uiWidth, 0.0f, frame::g_uiHeight, -1.0f, 1.0f);
    auto* sh = &s_shFontBitmap;
    ShaderUse(sh);

    texture::ImgBind(&s_tAsciiMap, GL_TEXTURE0);

    ShaderSetM4(sh, "uProj", proj);
    ShaderSetV4(sh, "uColor", {colors::hexToV4(0xeeeeeeff)});

    f64 currTime = utils::timeNowMS();
    if (currTime >= frame::g_prevTime + 1000.0)
    {
        String s = StringAlloc((IAllocator*)pAlloc, s_textFPS.maxSize);
        s.size = print::toString(&s, "FPS: {}\nFrame time: {:.3} ms", frame::g_nfps, frame::g_frameTime);

        frame::g_nfps = 0;
        frame::g_prevTime = currTime;

        text::BitmapUpdate(&s_textFPS, &pAlloc->super, s, 0, 0);
    }

    text::BitmapDraw(&s_textFPS);
}

static void
drawFPSCounterTTF(Arena* pAlloc)
{
    math::M4 proj = math::M4Ortho(0.0f, frame::g_uiWidth, 0.0f, frame::g_uiHeight, -1.0f, 1.0f);

    auto* sh = &s_sh1Col;
    ShaderUse(sh);

    ShaderSetM4(sh, "uProj", proj);
    ShaderSetV4(sh, "uColor", colors::hexToV4(0x00ff00ff));

    texture::ImgBind(s_ttfTest.texId, GL_TEXTURE0);

    static int nLastFps = frame::g_nfps;

    f64 currTime = utils::timeNowMS();
    if (currTime >= frame::g_prevTime + 1000.0)
        nLastFps = frame::g_nfps; 

    String s = StringAlloc((IAllocator*)pAlloc, s_ttfTest.maxSize);
    s.size = print::toString(&s, "FPS: {}\nFrame time: {:.3} ms", nLastFps, frame::g_frameTime);

    text::TTFUpdateText(&s_ttfTest, &pAlloc->super, s, 0, 0, 1.0f);

    if (currTime >= frame::g_prevTime + 1000.0)
    {
        frame::g_nfps = 0;
        frame::g_prevTime = currTime;
    }

    text::TTFDraw(&s_ttfTest);
}

static void
drawInfo(Arena* pArena)
{
    math::M4 proj = math::M4Ortho(0.0f, frame::g_uiWidth, 0.0f, frame::g_uiHeight, -1.0f, 1.0f);
    auto* sh = &s_sh1Col;
    ShaderUse(sh);

    ShaderSetM4(sh, "uProj", proj);
    ShaderSetV4(sh, "uColor", {colors::hexToV4(0x666666ff)});

    texture::ImgBind(s_ttfTest.texId, GL_TEXTURE0);

    String s = StringAlloc(&pArena->super, 256);
    s.size = print::toString(&s,
        "TOGGLE FULLSCREEN: F\n"
        "UNLOCK MOUSE: Q\n"
        "QUIT: ESC\n"
    );
    int nSpaces = 0;
    for (auto c : s) if (c == '\n') ++nSpaces;

    text::TTFUpdateText(&s_ttfTest, &pArena->super, s, 0, frame::g_uiHeight - (nSpaces*2), 1.0f);
    text::TTFDraw(&s_ttfTest);
}

static void
drawEntities([[maybe_unused]] Arena* pArena, const f64 alpha)
{
    ShaderUse(&s_shSprite);
    GLuint idxLastTex = 0;

    for (const Entity& en : g_aEntities)
    {
        if (en.bDead || en.eColor == COLOR::INVISIBLE) continue;

        auto enIdx = PoolIdx(&g_aEntities, &en);

        const auto& prev = s_aPrevPos[enIdx];
        math::V2 pos = math::lerp(
            tilePosToImagePos(prev.x, prev.y),
            tilePosToImagePos(en.pos.x, en.pos.y),
            alpha
        );

        pos = tilePosToImagePos(en.pos.x, en.pos.y);
        math::V2 off = tilePosToImagePos(en.xOff, en.yOff);

        math::M4 tm = math::M4Iden();
        tm = M4Translate(tm, {pos.x + off.x, pos.y + off.y, 0.0f + en.zOff});
        tm = M4Scale(tm, {frame::g_unit.first * en.width, frame::g_unit.second * en.height, 1.0f});

        if (idxLastTex != en.texIdx)
        {
            idxLastTex = en.texIdx;
            texture::ImgBind(en.texIdx, GL_TEXTURE0);
        }

        ShaderSetM4(&s_shSprite, "uModel", tm);
        ShaderSetV3(&s_shSprite, "uColor", blockColorToV3(en.eColor));
        PlainDraw(&s_plain);
    }
}

void
cleanup()
{
    PlainDestroy(&s_plain);

    for (auto& e : g_aAllShaders) ShaderDestroy(&e);

    for (auto& t : texture::g_aAllTextures) texture::ImgDestroy(&t);
    MapDestroy(&texture::g_mAllTexturesIdxs);

    AllocatorPoolFreeAll(&s_assetArenas);
}

} /* namespace game */
