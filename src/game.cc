#include "game.hh"

#include "Model.hh"
#include "Shader.hh"
#include "Window.hh"
#include "adt/AllocatorPool.hh"
#include "adt/Arena.hh"
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

static AllocatorPool<Arena> s_assetArenas(10);

static Vec<Entity> s_aEntities(AllocatorPoolGet(&s_assetArenas, SIZE_8K));
static Vec<game::Block> s_aBlocks(AllocatorPoolGet(&s_assetArenas, SIZE_1K));

static Shader s_shFontBitmap;
static Shader s_shSprite;
static Shader s_sh1Col;

static texture::Img s_tAsciiMap(AllocatorPoolGet(&s_assetArenas, SIZE_1M));
static texture::Img s_tBox(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tBall(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tPaddle(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 100));
static texture::Img s_tWhitePixel(AllocatorPoolGet(&s_assetArenas, 250));

static parser::Wave s_sndBeep(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 400));
static parser::Wave s_sndUnatco(AllocatorPoolGet(&s_assetArenas, SIZE_1M * 35));

static Plain s_plain;

static parser::ttf::Font s_fontLiberation(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 500));
static text::Bitmap s_textFPS;
static text::TTF s_ttfTest(AllocatorPoolGet(&s_assetArenas, SIZE_1K * 520));

Player g_player {
    .enIdx = 0,
    .speed = 0.8,
    .dir {},
};

Ball g_ball {
    .enIdx = 0,
    .bReleased = false,
    .speed = 0.8f,
    .radius = 20.0f,
    .dir {},
};

static void drawFPSCounter(Arena* pAlloc);
static void drawFPSCounterTTF(Arena* pAlloc);
static void drawInfo(Arena* pArena);
static void drawEntities(Arena* pAlloc);
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
    return s_aEntities[e.enIdx].pos + (dir * (frame::g_deltaTimeS * e.speed));
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
    for (int i = 0; i < 4; i++)
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
    namespace f = frame;

    bool bAddSound = false;

    for (auto& block : s_aBlocks)
    {
        auto& b = s_aEntities[block.enIdx];

        if (b.bDead || b.eColor == COLOR::INVISIBLE) continue;

        math::V2 center = nextPos(g_ball, true);
        math::V2 aabbHalfExtents {f::g_unit.x / 2, f::g_unit.y / 2};
        math::V2 aabbCenter = b.pos;
        math::V2 diff = center - b.pos;
        math::V2 clamped = math::V2Clamp(diff, -aabbHalfExtents, aabbHalfExtents);
        math::V2 closest = aabbCenter + clamped;
        diff = closest - center;
        /*auto diffLen = math::V2Length(diff);*/

        const auto& bx = s_aEntities[g_ball.enIdx].pos.x;
        const auto& by = s_aEntities[g_ball.enIdx].pos.y;

        const auto& ex = b.pos.x;
        const auto& ey = b.pos.y;

        /*if (diffLen <= g_ball.radius)*/
        if (bx >= ex - f::g_unit.x - 4 && bx <= ex + f::g_unit.x + 4 &&
            by >= ey - f::g_unit.y - 4 && by <= ey + f::g_unit.y + 4)
        {
            if (b.eColor != COLOR::INVISIBLE && b.eColor != COLOR::DIMGRAY) b.bDead = true;

            bAddSound = true;

            auto side = getReflectionSide(diff);

            /* FIXME: sides are flipped */
            auto& enBall = s_aEntities[g_ball.enIdx];
            f32 off = 16.0f;
            switch (side)
            {
                default: break;

                case REFLECT_SIDE::UP:
                {
                    /*enBall.pos.y -= f::g_unit.y / off;*/
                    g_ball.dir.y = -g_ball.dir.y;
                } break;

                case REFLECT_SIDE::DOWN:
                {
                    /*enBall.pos.y += f::g_unit.y / off;*/
                    g_ball.dir.y = -g_ball.dir.y;
                } break;

                case REFLECT_SIDE::LEFT:
                {
                    enBall.pos.x += f::g_unit.x / off;
                    if (math::eq(g_ball.dir.x, 0))
                    {
                        g_ball.dir.x = 0.1f;
                        break;
                    }

                    g_ball.dir.x = -g_ball.dir.x;
                } break;

                case REFLECT_SIDE::RIGHT:
                {
                    /*enBall.pos.x -= f::g_unit.x / off;*/
                    if (math::eq(g_ball.dir.x, 0))
                    {
                        g_ball.dir.x = -0.1f;
                        break;
                    }

                    g_ball.dir.x = -g_ball.dir.x;
                } break;
            }
        }
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

static void
paddleHit()
{
    namespace f = frame;
    auto& enBall = s_aEntities[g_ball.enIdx];
    auto& enPlayer = s_aEntities[g_player.enIdx];

    const auto& bx = enBall.pos.x;
    const auto& by = enBall.pos.y;
    const auto& px = enPlayer.pos.x;
    const auto& py = enPlayer.pos.y;

    if (bx >= px - f::g_unit.x*2.0f && bx <= px + f::g_unit.x*2.0f &&
        by >= py - f::g_unit.y && by <= py - f::g_unit.y + f::g_unit.y/2.0f)
    {
        enBall.pos.y = (py - f::g_unit.y + f::g_unit.y/2.0f) + 4.0f;
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));

        g_ball.dir.y = 1.0f;
        if (math::V2Length(g_player.dir) > 0.0f)
            g_ball.dir += g_player.dir * 0.1f;
    }
}

static void
outOfBounds()
{
    namespace f = frame;

    auto& enBall = s_aEntities[g_ball.enIdx];

    /* out of bounds */
    bool bAddSound = false;
    if (enBall.pos.y <= 0.0f - f::g_unit.y) 
    {
        g_ball.bReleased = false;
        // g_ball.base.pos.y = 0.0f - f::g_unit.y;
        // g_ball.dir.y = 1.0f;
        // bAddSound = true;
    }
    else if (enBall.pos.y >= f::HEIGHT - f::g_unit.y)
    {
        enBall.pos.y = f::HEIGHT - f::g_unit.y - 4;
        g_ball.dir.y = -1.0f;
        bAddSound = true;
    }
    else if (enBall.pos.x <= 0.0f - f::g_unit.x)
    {
        enBall.pos.x = -f::g_unit.x + 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }
    else if (enBall.pos.x >= f::WIDTH - f::g_unit.x)
    {
        enBall.pos.x = f::WIDTH - f::g_unit.x - 4;
        g_ball.dir.x = -g_ball.dir.x;
        bAddSound = true;
    }

    if (bAddSound)
        audio::MixerAdd(app::g_pMixer, parser::WaveGetTrack(&s_sndBeep, false, 1.2f));
}

static inline math::V2
tilePosToImagePos(u32 x, u32 y)
{
    namespace f = frame;
    return {f::g_unit.x*2 * y, (f::HEIGHT - f::g_unit.y*2) - f::g_unit.y*2 * x};
}

void
loadLevel()
{
    const auto& lvl = g_lvl1;
    const auto& l = lvl.aTiles;

    auto at = [&](int y, int x) -> s8& {
        return l[y*lvl.width + x];
    };

    const u32 levelY = lvl.height;
    const u32 levelX = lvl.width;

    frame::g_unit.x = frame::WIDTH / levelX / 2;
    frame::g_unit.y = frame::HEIGHT / levelY / 2;

    VecSetCap(&s_aBlocks, levelY*levelX);
    VecSetSize(&s_aBlocks, 0);

    auto fBoxTex = MapSearch(&texture::g_mAllTexturesIdxs, {"test-assets/box3.bmp"});
    auto boxTexId = texture::g_aAllTextures[fBoxTex.pData->vecIdx].id;

    for (u32 i = 0; i < levelY; i++)
    {
        for (u32 j = 0; j < levelX; j++)
        {
            if (at(i, j) != s8(COLOR::INVISIBLE))
            {
                u32 idx = VecPush(&s_aEntities, {});
                auto& e = s_aEntities[idx];

                VecPush(&s_aBlocks, {u16(idx)});
                e.pos = tilePosToImagePos(i, j);
                e.width = 1.0f;
                e.height = 1.0f;
                e.xOff = 0.0f;
                e.yOff = 0.0f;
                e.zOff = 0.0f;
                e.shaderIdx = 0;
                e.texIdx = u16(boxTexId);
                e.eColor = COLOR(at(i, j));
                e.bDead = false;
                e.bRemoveAfterDraw = false;
            }
        }
    }

    g_player.enIdx = VecPush(&s_aEntities, {});
    auto& enPlayer = s_aEntities[g_player.enIdx];
    enPlayer.pos.x = frame::WIDTH/2 - frame::g_unit.x;
    enPlayer.texIdx = s_tPaddle.id;
    enPlayer.width = 2.0f;
    enPlayer.height = 1.0f;
    enPlayer.xOff = -frame::g_unit.x;
    enPlayer.zOff = 10.0f;
    enPlayer.eColor = COLOR::TEAL;
    enPlayer.bRemoveAfterDraw = false;

    g_ball.enIdx = VecPush(&s_aEntities, {});
    auto& enBall = s_aEntities[g_ball.enIdx];
    enBall.eColor = COLOR::ORANGERED;
    enBall.texIdx = s_tBall.id;
    enBall.width = 1.0f;
    enBall.height = 1.0f;
    enBall.zOff = 10.0f;
    enBall.bRemoveAfterDraw = false;

    audio::MixerAddBackground(app::g_pMixer, parser::WaveGetTrack(&s_sndUnatco, true, 0.7f));
}

void
updateState()
{
    namespace f = frame;
    auto& enBall = s_aEntities[g_ball.enIdx];
    auto& enPlayer = s_aEntities[g_player.enIdx];

    /* player */
    {
        enPlayer.pos = nextPos(g_player, false);

        if (enPlayer.pos.x >= f::WIDTH - f::g_unit.x*2)
        {
            enPlayer.pos.x = f::WIDTH - f::g_unit.x*2;
            g_player.dir = {};
        }
        else if (enPlayer.pos.x <= 0)
        {
            enPlayer.pos.x = 0;
            g_player.dir = {};
        }
    }

    /* ball */
    {
        if (g_ball.bReleased)
        {
            blockHit();
            paddleHit();
            outOfBounds();
            enBall.pos = nextPos(g_ball, true);

        } else enBall.pos = enPlayer.pos;

        const auto& pos = enBall.pos;

        math::M4 tm;
        tm = math::M4Iden();
        tm = M4Translate(tm, {pos.x, pos.y, 10.0f});
        tm = M4Scale(tm, {f::g_unit.x, f::g_unit.y, 1.0f});
    }
}

void
draw(Arena* pArena)
{
    drawEntities(pArena);

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
        String s = StringAlloc((Allocator*)pAlloc, s_textFPS.maxSize);
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
    {
        nLastFps = frame::g_nfps; 
    }

    String s = StringAlloc((Allocator*)pAlloc, s_ttfTest.maxSize);
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
drawEntities([[maybe_unused]] Arena* pAlloc)
{
    ShaderUse(&s_shSprite);
    GLuint idxLastTex = 0;

    for (const Entity& e : s_aEntities)
    {
        if (e.bDead || e.eColor == COLOR::INVISIBLE) continue;

        math::M4 tm = math::M4Iden();
        tm = math::M4Iden();
        tm = M4Translate(tm, {e.pos.x + e.xOff, e.pos.y + e.yOff, 0.0f + e.zOff});
        tm = M4Scale(tm, {frame::g_unit.x * e.width, frame::g_unit.y * e.height, 1.0f});

        if (idxLastTex != e.texIdx)
        {
            idxLastTex = e.texIdx;
            texture::ImgBind(e.texIdx, GL_TEXTURE0);
        }

        ShaderSetM4(&s_shSprite, "uModel", tm);
        ShaderSetV3(&s_shSprite, "uColor", blockColorToV3(e.eColor));
        PlainDraw(&s_plain);
    }
}

void
cleanup()
{
    PlainDestroy(&s_plain);

    for (auto& e : g_aAllShaders)
        ShaderDestroy(&e);
    VecDestroy(&g_aAllShaders);

    for (auto& t : texture::g_aAllTextures)
        texture::ImgDestroy(&t);
    VecDestroy(&texture::g_aAllTextures);
    MapDestroy(&texture::g_mAllTexturesIdxs);

    AllocatorPoolFreeAll(&s_assetArenas);
}

} /* namespace game */
