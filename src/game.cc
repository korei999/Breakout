#include "game.hh"

#include "AllocatorPool.hh"
#include "IWindow.hh"
#include "Shader.hh"
#include "adt/Arena.hh"
#include "adt/Pool.hh"
#include "adt/ScratchBuffer.hh"
#include "adt/ThreadPool.hh"
#include "adt/Span2D.hh"
#include "adt/defer.hh"
#include "app.hh"
#include "controls.hh"
#include "frame.hh"
#include "reader/Wave.hh"
#include "reader/ttf.hh"
#include "text.hh"
#include "texture.hh"

namespace game
{

struct WidthHeight
{
    f32 width {};
    f32 height {};
};

thread_local static u8 tls_aMemBuffer[SIZE_8K] {};
thread_local static ScratchBuffer tls_scratch(tls_aMemBuffer);

static AllocatorPool<Arena, ASSET_MAX_COUNT> s_assetArenas {};

static Vec<game::Block> s_aBlocks(s_assetArenas.get(SIZE_1K));

static Shader s_shFontBitmap;
static Shader s_shSprite;
static Shader s_sh1Col;

static texture::Img s_tAsciiMap(s_assetArenas.get(SIZE_1M));
static texture::Img s_tBox(s_assetArenas.get(SIZE_1K * 100));
static texture::Img s_tBall(s_assetArenas.get(SIZE_1K * 100));
static texture::Img s_tPaddle(s_assetArenas.get(SIZE_1K * 100));
static texture::Img s_tWhitePixel(s_assetArenas.get(250));

static reader::Wave s_sndBeep(s_assetArenas.get(SIZE_1K * 400));
static reader::Wave s_sndUnatco(s_assetArenas.get(SIZE_1M * 35));

static Plain s_plain;

static text::TTF s_ttfWriter(s_assetArenas.get(SIZE_1K * 520));
static reader::ttf::Font s_fontLiberation(s_assetArenas.get(SIZE_1K * 500));

Pool<Entity, ASSET_MAX_COUNT> g_aEntities;
static Arr<math::V2, ASSET_MAX_COUNT> s_aPrevPos;

static const Level* s_pCurrLvl {};
static WidthHeight s_currLvlSize {};
static Map<Entity*, Pair<u16, u16>> s_mapPEntityToTilePos(s_assetArenas.get(SIZE_1K));
static Vec<Entity*> s_aPBlocksMap(s_assetArenas.get(SIZE_1K));

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
static void drawInfo(Arena* pArena);
static void drawEntities(Arena* pAlloc, const f64 alpha);
static void drawTTFTest(Arena* pAlloc);

void
loadAssets()
{
    f64 t0 = utils::timeNowS();
    LOG_GOOD("loadAssets() at: {}\n", (ssize)t0);

    frame::g_uiHeight = (frame::g_uiWidth * (f32)app::g_pWindow->m_wHeight) / (f32)app::g_pWindow->m_wWidth;

    s_plain = Plain(GL_STATIC_DRAW);

    s_shFontBitmap.load("shaders/font/font.vert", "shaders/font/font.frag");
    s_shFontBitmap.use();
    s_shFontBitmap.setI("tex0", 0);

    s_sh1Col.load("shaders/font/1col.vert", "shaders/font/1col.frag");
    s_sh1Col.use();
    s_sh1Col.setI("uTex0", 0);

    s_shSprite.load("shaders/2d/sprite.vert", "shaders/2d/sprite.frag");
    s_shSprite.use();
    s_shSprite.setI("tex0", 0);

    frame::g_uboProjView.bindShader(&s_shSprite, "ubProjView", 0);

    s_fontLiberation.loadParse("test-assets/LiberationMono-Regular.ttf");

    /* unbind before running threads */
    app::g_pWindow->unbindGlContext();
    defer( app::g_pWindow->bindGlContext() );

    text::TTFRasterizeArg argTTF {&s_ttfWriter, &s_fontLiberation};

    reader::WaveLoadArg argBeep {&s_sndBeep, "test-assets/c100s16.wav"};
    reader::WaveLoadArg argUnatco {&s_sndUnatco, "test-assets/Unatco.wav"};

    texture::ImgLoadArg argFontBitmap {&s_tAsciiMap, "test-assets/bitmapFont20.bmp"};
    texture::ImgLoadArg argBox {&s_tBox, "test-assets/box3.bmp"};
    texture::ImgLoadArg argBall {&s_tBall, "test-assets/ball.bmp"};
    texture::ImgLoadArg argPaddle {&s_tPaddle, "test-assets/paddle.bmp"};
    texture::ImgLoadArg argWhitePixel {&s_tWhitePixel, "test-assets/WhitePixel.bmp"};

    app::g_pThreadPool->submit(text::TTFRasterizeSubmit, &argTTF);

    app::g_pThreadPool->submit(reader::WaveSubmit, &argBeep);
    app::g_pThreadPool->submit(reader::WaveSubmit, &argUnatco);

    app::g_pThreadPool->submit(texture::ImgSubmit, &argFontBitmap);
    app::g_pThreadPool->submit(texture::ImgSubmit, &argBox);
    app::g_pThreadPool->submit(texture::ImgSubmit, &argBall);
    app::g_pThreadPool->submit(texture::ImgSubmit, &argPaddle);
    app::g_pThreadPool->submit(texture::ImgSubmit, &argWhitePixel);

    app::g_pThreadPool->wait();

    f64 t1 = utils::timeNowS();
    LOG_GOOD("loaded in: {} s, at {}\n", t1 - t0, (ssize)t1);
}

template<typename T>
static inline math::V2
nextPos(const T& e, bool bNormalizeDir)
{
    auto dir = bNormalizeDir ? math::normalize(e.dir) : e.dir;
    return e.pos + (dir * (frame::g_dt * e.speed));
}

[[maybe_unused]] static REFLECT_SIDE
getReflectionSide(math::V2 tar)
{
    constexpr math::V2 compass[] {
        { 0.0,  1.0}, /* up */
        { 1.0,  0.0}, /* right */
        { 0.0, -1.0}, /* down */
        {-1.0,  0.0}, /* left */
    };

    f32 max = 0.0f;
    REFLECT_SIDE eBestMatch = NONE;
    for (u32 i = 0; i < utils::size(compass); ++i)
    {
        f32 dot = V2Dot(V2Norm(tar), compass[i]);
        if (dot > max || math::eq(dot, max))
        {
            max = dot;
            eBestMatch = REFLECT_SIDE(i);
        }
    }

    if (eBestMatch != REFLECT_SIDE::NONE)
    {
        /* DBG */
        LOG("max: {:.10}, eBestMatch: {}\n", max, eBestMatch);
        if (max < 0.99)
        {
            /*eBestMatch = REFLECT_SIDE::CORNER;*/
        }
    }

    return eBestMatch;
}

static REFLECT_SIDE
getReflectionSideV2(const Entity& e)
{
    auto& enBall = g_aEntities[g_ball.enIdx];
    REFLECT_SIDE eSide = NONE;

    if (enBall.pos.y < e.pos.y - (e.height / 2.0f))
        eSide = DOWN;
    else if (enBall.pos.y > e.pos.y + (e.height / 2.0f))
        eSide = UP;
    else if (enBall.pos.x < e.pos.x)
        eSide = LEFT;
    else if (enBall.pos.x > e.pos.x)
        eSide = RIGHT;

    return eSide;
}

static bool
AABB(
    const math::V2 lPos, const f32 lWidth, const f32 lHeight,
    const math::V2 rPos, const f32 rWidth, const f32 rHeight
)
{
    if (
        lPos.x >= rPos.x - lWidth/2.0f - rWidth/2.0f  && lPos.x <= rPos.x + lWidth/2.0f + rWidth/2.0f &&
        lPos.y >= rPos.y - lHeight/2.0f - rHeight/2.0f && lPos.y <= rPos.y + lHeight/2.0f + rHeight/2.0f
    )
        return true;
    else return false;
}

static void
explodeBlockDFS(Vec<Entity*>* pVDfsMap, Entity* pEntity)
{
    const u32 lvlWidth = s_pCurrLvl->width;
    const u32 lvlHeight = s_pCurrLvl->height;

    auto fPos = s_mapPEntityToTilePos.search(pEntity);
    if (!fPos) return;

    const auto& [tileX, tileY] = fPos.data().val;

    /* xy offsets */
    constexpr Pair<s8, s8> aKernel[] {
        {-1,  1}, {0,  1}, {1,  1},
        {-1,  0},          {1,  0},
        {-1, -1}, {0, -1}, {1, -1},
    };

    Span2D span(s_aPBlocksMap.data(), lvlWidth, lvlHeight);
    Span2D dfsMap(pVDfsMap->data(), lvlWidth, lvlHeight);

    for (auto [x, y] : aKernel)
    {
        u32 px = int(tileX) + x;
        u32 py = int(tileY) + y;

        if (px < lvlWidth && py < lvlHeight)
        {
            auto* pEn = span(px, py);
            if (pEn && !dfsMap(px, py))
            {
                dfsMap(px, py) = pEn;

                if (pEn->eColor == game::COLOR::RED)
                    explodeBlockDFS(pVDfsMap, pEn);
            }
        }
    }
}

static void
explodeBlock(Arena* pArena, Entity* p)
{
    const u32 lvlWidth = s_pCurrLvl->width;
    const u32 lvlHeight = s_pCurrLvl->height;

    Vec<Entity*> vDfsMap(pArena, lvlWidth * lvlHeight);
    vDfsMap.setSize(lvlWidth * lvlHeight);

    explodeBlockDFS(&vDfsMap, p);

    for (auto pEn : vDfsMap)
        if (pEn) pEn->bDead = true;
}

static void
blockHit(Arena* pArena)
{
    auto& mix = *app::g_pMixer;
    bool bAddSound = false;
    bool bExplosive = false;

    defer(
        if (bAddSound)
        {
            f32 vol = 1.0f;
            if (bExplosive) vol *= 1.6;

            mix.add(s_sndBeep.getTrack(false, vol));
        }
    );

    auto& enBall = g_aEntities[g_ball.enIdx];
    for (auto& block : s_aBlocks)
    {
        auto& b = g_aEntities[block.enIdx];

        bool bMarkDead = false;
        defer(
            if (bMarkDead)
                b.bDead = true;
        );

        if (b.bDead || b.eColor == game::COLOR::INVISIBLE) continue;

        math::V2 center = nextPos(enBall, true);
        if (AABB(center, g_ball.radius, g_ball.radius, b.pos, b.width, b.height))
        {
            if (g_ball.bCollided)
            {
                g_ball.bCollided = false;
                break;
            }

            bAddSound = true;
            g_ball.bCollided = true;

            auto eSide = getReflectionSideV2(b);

            auto& enBall = g_aEntities[g_ball.enIdx];
            switch (eSide)
            {
                case REFLECT_SIDE::NONE:
                case REFLECT_SIDE::ELAST:
                case REFLECT_SIDE::CORNER:
                break;

                case REFLECT_SIDE::UP:
                {
                    enBall.dir.y = -enBall.dir.y;
                }
                break;

                case REFLECT_SIDE::DOWN:
                {
                    enBall.dir.y = -enBall.dir.y;
                }
                break;

                case REFLECT_SIDE::LEFT:
                {
                    enBall.dir.x = -enBall.dir.x;
                }
                break;

                case REFLECT_SIDE::RIGHT:
                {
                    enBall.dir.x = -enBall.dir.x;
                }
                break;
            }

            if (b.eColor != game::COLOR::INVISIBLE && b.eColor != game::COLOR::DIMGRAY)
                bMarkDead = true;

            if (b.eColor == game::COLOR::RED)
            {
                enBall.dir = enBall.pos - b.pos;
                bExplosive = true;

                explodeBlock(pArena, &b);
            }

            break;
        }
    }
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

        app::g_pMixer->add(s_sndBeep.getTrack(false, 1.0f));

        utils::negate(&enBall.dir.y);
        if (math::V2Length(enPlayer.dir) > 0.0f)
            enBall.dir += enPlayer.dir * 0.1f;
    }
}

static void
outOfBounds()
{
    auto& enBall = g_aEntities[g_ball.enIdx];

    bool bAddSound = false;
    if (enBall.pos.y < -0.5f)
    {
        if (controls::g_bStepDebug)
        {
            enBall.pos.y = -0.5f;
            utils::negate(&enBall.dir.y);
            bAddSound = true;
        }
        else g_ball.bReleased = false;
    }
    else if (enBall.pos.y > s_currLvlSize.height - 0.5f)
    {
        enBall.pos.y = s_currLvlSize.height - 0.5f;
        utils::negate(&enBall.dir.y);
        bAddSound = true;
    }
    else if (enBall.pos.x < -0.5f)
    {
        enBall.pos.x = -0.5f;
        utils::negate(&enBall.dir.x);
        bAddSound = true;
    }
    else if (enBall.pos.x > s_currLvlSize.width - 0.5f)
    {
        enBall.pos.x = s_currLvlSize.width - 0.5f;
        utils::negate(&enBall.dir.x);
        bAddSound = true;
    }

    if (bAddSound)
        app::g_pMixer->add(s_sndBeep.getTrack(false, 1.0f));
}

static inline math::V2
tileToImage(const f32 x, const f32 y)
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
    const auto& aTiles = lvl.aTiles;
    s_pCurrLvl = &lvl;

    Span2D lvlAt(aTiles, lvl.width, lvl.height);

    frame::g_unit.first = frame::WIDTH / lvl.width / 2;
    frame::g_unit.second = frame::HEIGHT / lvl.height / 2;

    s_aBlocks.setCap(lvl.height*lvl.width);
    s_aBlocks.setSize(0);

    auto fBoxTex = texture::g_mAllTexturesIdxs.search("test-assets/box3.bmp");
    assert(fBoxTex);
    auto boxTexId = texture::g_aAllTextures[fBoxTex.data().val].m_id;

    s_aPBlocksMap.setSize(lvl.width * lvl.height);
    s_mapPEntityToTilePos.zeroOut();

    LOG_NOTIFY("width: {}, height: {}\n", lvl.width, lvl.height);
    for (u32 y = 0; y < lvl.height; ++y)
    {
        for (u32 x = 0; x < lvl.width; ++x)
        {
            if (lvlAt(x, y) != s8(game::COLOR::INVISIBLE))
            {
                u32 idx = g_aEntities.getHandle();
                auto& e = g_aEntities[idx];

                s_aBlocks.emplace(u16(idx));
                e.pos = {f32(x), f32(lvl.height - y - 1)};
                e.width = 1.0f;
                e.height = 1.0f;
                e.xOff = 0.0f;
                e.yOff = 0.0f;
                e.zOff = 0.0f;
                e.shaderIdx = 0;
                e.texIdx = u16(boxTexId);
                e.eColor = game::COLOR(lvlAt(x, y));
                e.bDead = false;
                e.bRemoveAfterDraw = false;

                s_mapPEntityToTilePos.emplace(&e, x, y);
                s_aPBlocksMap[y*lvl.width + x] = &e;
            }
        }
    }

    g_player.enIdx = g_aEntities.getHandle();
    auto& enPlayer = g_aEntities[g_player.enIdx];
    enPlayer.speed = 9.0f;
    enPlayer.pos.x = lvl.width / 2.0f;
    enPlayer.texIdx = s_tPaddle.m_id;
    enPlayer.width = 2.0f;
    enPlayer.height = 1.0f;
    enPlayer.xOff = -0.5f;
    enPlayer.zOff = 10.0f;
    enPlayer.eColor = game::COLOR::TEAL;
    enPlayer.bRemoveAfterDraw = false;

    g_ball.enIdx = g_aEntities.getHandle();
    auto& enBall = g_aEntities[g_ball.enIdx];
    enBall.speed = 9.0f;
    enBall.eColor = game::COLOR::ORANGERED;
    enBall.texIdx = s_tBall.m_id;
    enBall.width = 1.0f;
    enBall.height = 1.0f;
    enBall.zOff = 10.0f;
    enBall.bRemoveAfterDraw = false;

    app::g_pMixer->addBackground(s_sndUnatco.getTrack(true, 0.7f));

    s_aPrevPos.setSize(0);
    for (auto& en : g_aEntities)
        s_aPrevPos.push(en.pos);

    s_currLvlSize.width = lvl.width;
    s_currLvlSize.height = lvl.height;
}

void
updateState(Arena* pArena)
{
    auto& enBall = g_aEntities[g_ball.enIdx];
    auto& enPlayer = g_aEntities[g_player.enIdx];

    /* keep prev positions */
    {
        for (auto& en : g_aEntities)
        {
            auto idx = g_aEntities.idx(&en);
            s_aPrevPos[idx] = en.pos;
        }
    }

    /* player */
    {
        enPlayer.pos = nextPos(enPlayer, false);

        if (enPlayer.pos.x > s_currLvlSize.width - 1.0f)
        {
            enPlayer.pos.x = s_currLvlSize.width - 1.0f;
            enPlayer.dir = {};
        }
        else if (enPlayer.pos.x < 0)
        {
            enPlayer.pos.x = 0;
            enPlayer.dir = {};
        }
    }

    /* ball */
    {
        if (g_ball.bReleased)
        {
            blockHit(pArena);
            paddleHit();
            outOfBounds();
            enBall.pos = nextPos(enBall, true);

        } else enBall.pos = enPlayer.pos;
    }
}

void
draw(Arena* pArena, const f64 alpha)
{
    if (controls::g_bTTFDebugScreen)
    {
        drawTTFTest(pArena);
    }
    else
    {
        drawEntities(pArena, alpha);
    }

    drawFPSCounter(pArena);
    drawInfo(pArena);
}

static void
drawFPSCounter(Arena* pAlloc)
{
    namespace f = frame;

    auto width = f::g_uiWidth;
    auto height = f::g_uiHeight;

    static int nLastFps = f::g_nfps;

    f64 currTime = utils::timeNowMS();
    if ((currTime - f::g_prevTime) >= 1000.0)
        nLastFps = f::g_nfps; 

    auto sp = tls_scratch.nextMemZero<char>(s_ttfWriter.m_maxSize);
    ssize nChars = print::toSpan(sp, "FPS: {}\nFrame time: {:.3} ms", nLastFps, f::g_frameTime);

    s_ttfWriter.updateText(pAlloc, String(sp.data(), nChars), 0.0f, height - 2.0f, 1.0f);

    if (currTime >= f::g_prevTime + 1000.0)
    {
        f::g_nfps = 0;
        f::g_prevTime = currTime;
    }

    math::M4 proj = math::M4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    auto* sh = &s_sh1Col;

    sh->use();
    sh->setM4("uProj", proj);
    sh->setV4("uColor", colors::hexToV4(0x00ff00ff));
    texture::ImgBind(s_ttfWriter.m_texId, GL_TEXTURE0);

    s_ttfWriter.draw();
}

static void
drawTTFTest(Arena* pAlloc)
{
    const f32 width = frame::g_uiWidth / 2.0f;
    const f32 height = frame::g_uiHeight / 2.0f;
    const math::M4 proj = math::M4Ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    auto sp = tls_scratch.nextMemZero<char>(s_ttfWriter.m_maxSize);

    int i, j;
    int off = 0;
    for (i = '!', j = 0; i <= '~' && j < (int)s_ttfWriter.m_maxSize; ++i, ++j)
    {
        if (j % int(width) == 0)
            sp[j++] = '\n';

        sp[j] = i;
    }

    s_ttfWriter.updateText(pAlloc, {sp.data(), j}, 0, height/2.0f + 2.0f, 1.0f);

    auto* sh = &s_sh1Col;

    sh->use();
    sh->setM4("uProj", proj);
    sh->setV4("uColor", colors::hexToV4(0xeeeeeeff));
    texture::ImgBind(s_ttfWriter.m_texId, GL_TEXTURE0);

    s_ttfWriter.draw();
}

static void
drawInfo(Arena* pArena)
{
    math::M4 proj = math::M4Ortho(0.0f, frame::g_uiWidth, 0.0f, frame::g_uiHeight, -1.0f, 1.0f);
    auto* sh = &s_sh1Col;
    sh->use();

    sh->setM4("uProj", proj);
    sh->setV4("uColor", {colors::hexToV4(0x666666ff)});

    texture::ImgBind(s_ttfWriter.m_texId, GL_TEXTURE0);

    auto sp = tls_scratch.nextMem<char>(256);
    ssize nChars = print::toSpan(sp,
        "Fullscreen: F\n"
        "Mouse lock: Q\n"
        "Quit: ESC\n"
    );
    String s = {sp.data(), nChars};

    int nSpaces = 0;
    for (auto c : s) if (c == '\n') ++nSpaces;

    s_ttfWriter.updateText(pArena, s, 0, nSpaces + 1.0f, 1.0f);
    s_ttfWriter.draw();
}

static void
drawEntities([[maybe_unused]] Arena* pArena, const f64 alpha)
{
    s_shSprite.use();
    GLuint idxLastTex = 0;

    for (const Entity& en : g_aEntities)
    {
        if (en.bDead || en.eColor == game::COLOR::INVISIBLE) continue;

        auto enIdx = g_aEntities.idx(&en);

        math::V2 pos;

        if (controls::g_bStepDebug)
        {
            pos = tileToImage(en.pos.x, en.pos.y);
        }
        else
        {
            const auto& prevPos = s_aPrevPos[enIdx];
            pos = math::lerp(
                tileToImage(prevPos.x, prevPos.y),
                tileToImage(en.pos.x, en.pos.y),
                alpha
            );
        }

        math::V2 off = tileToImage(en.xOff, en.yOff);

        math::M4 tm = math::M4Iden();
        tm = M4Translate(tm, {pos.x + off.x, pos.y + off.y, 0.0f + en.zOff});
        tm = M4Scale(tm, {frame::g_unit.first * en.width, frame::g_unit.second * en.height, 1.0f});

        if (idxLastTex != en.texIdx)
        {
            idxLastTex = en.texIdx;
            texture::ImgBind(en.texIdx, GL_TEXTURE0);
        }

        s_shSprite.setM4("uModel", tm);
        s_shSprite.setV3("uColor", blockColorToV3(en.eColor));
        s_plain.draw();
    }
}

void
cleanup()
{
    s_plain.destroy();

    for (auto& e : g_aAllShaders) e.destroy();

    for (auto& t : texture::g_aAllTextures) t.destroy();
    texture::g_mAllTexturesIdxs.destroy();

    s_assetArenas.freeAll();
}

} /* namespace game */
