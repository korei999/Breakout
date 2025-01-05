#include "text.hh"

#include "adt/Arena.hh"
#include "adt/Arr.hh"
#include "adt/Vec.hh"
#include "adt/defer.hh"
#include "app.hh"
#include "frame.hh"

using namespace adt;

namespace text
{

struct CharQuad
{
    f32 vs[24]; /* 6(2 triangles) * 4(2 pos, 2 uv coords) */
};

struct CharQuad3Pos2UV
{
    f32 vs[30]; /* 6(2 triangles) * 5(3 pos, 2 uv coords) */
};

struct Point
{
    f32 x, y, u, v;
};

struct PointOnCurve
{
    math::V2 pos;
    bool bOnCurve;
    bool bEndOfCurve;
    f32 __pad {};
};

static VecBase<CharQuad> updateBuffer(Bitmap* s, IAllocator* pAlloc, String str, u32 size, int xOrigin, int yOrigin);
static void genMesh(Bitmap* s, int xOrigin, int yOrigin, GLint drawMode);

Bitmap::Bitmap(String s, u64 size, int x, int y, GLint drawMode)
    : m_str(s), m_maxSize(size)
{
    genMesh(this, x, y, drawMode);
}

static void
genMesh(Bitmap* s, int xOrigin, int yOrigin, GLint drawMode)
{
    Arena arena(SIZE_1M);
    defer( arena.freeAll() );

    auto aQuads = updateBuffer(s, &arena, s->m_str, s->m_maxSize, xOrigin, yOrigin);

    glGenVertexArrays(1, &s->m_vao);
    glBindVertexArray(s->m_vao);

    glGenBuffers(1, &s->m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->m_vbo);
    glBufferData(GL_ARRAY_BUFFER, s->m_maxSize * sizeof(CharQuad), aQuads.data(), drawMode);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(math::V2), (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(math::V2), (void*)(sizeof(math::V2)));

    glBindVertexArray(0);
}

static VecBase<CharQuad>
updateBuffer(Bitmap* s, IAllocator* pAlloc, String str, u32 size, int xOrigin, int yOrigin)
{
    VecBase<CharQuad> aQuads(pAlloc, size);

    /* 16/16 bitmap aka extended ascii */
    auto getUV = [](int p) -> f32 {
        return (1.0f / 16.0f) * p;
    };

    f32 xOff = 0.0f;
    f32 yOff = 0.0f;
    for (char c : str)
    {
        /* tl */
        f32 x0 = getUV(c % 16);
        f32 y0 = getUV(16 - (c / 16));

        /* bl */
        f32 x1 = x0;
        f32 y1 = y0 - getUV(1);

        /* br */
        f32 x2 = x0 + getUV(1);
        f32 y2 = y1;

        /* tr */
        f32 x3 = x1 + getUV(1);
        f32 y3 = y0;

        if (c == '\n')
        {
            xOff = -0.0f;
            yOff -= 2.0f;
            continue;
        }

        aQuads.push(pAlloc, {
             0.0f + xOff + xOrigin,  2.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x0, y0, /* tl */
             0.0f + xOff + xOrigin,  0.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x1, y1, /* bl */
             2.0f + xOff + xOrigin,  0.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x2, y2, /* br */

             0.0f + xOff + xOrigin,  2.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x0, y0, /* tl */
             2.0f + xOff + xOrigin,  0.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x2, y2, /* br */
             2.0f + xOff + xOrigin,  2.0f + yOff + frame::g_uiHeight - 2.0f - yOrigin,     x3, y3, /* tr */
        });

        /* TODO: account for aspect ratio */
        xOff += 2.0f;
    }

    s->m_vboSize = aQuads.getSize() * 6; /* 6 vertices for 1 quad */

    return aQuads;
}

void
Bitmap::update(IAllocator* pAlloc, String str, int x, int y)
{
    assert(str.getSize() <= m_maxSize);

    this->m_str = str;
    VecBase<CharQuad> aQuads = updateBuffer(this, pAlloc, str, m_maxSize, x, y);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_maxSize * sizeof(f32)*4*6, aQuads.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
Bitmap::draw()
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vboSize);
}

static VecBase<Point>
getBeizerPoints(
    IAllocator* pAlloc,
    const math::V2& p0,
    const math::V2& p1,
    const math::V2& p2,
    int nSteps)
{
    /* quadratic bezier */
    /*B(t) = (1-t)^2*P0 + 2(1-t)*t*P1 + t^2*P2*/

    VecBase<Point> aPoints(pAlloc, 2 + nSteps);
    aPoints.push(pAlloc, {p0.x, p0.y, 0.0f, 1.0f});
    for (int i = 1; i < nSteps; i++) {
        f32 t = f32(i) / f32(nSteps);

        math::V2 vec = math::bezier(p0, p1, p2, t);

        aPoints.push(pAlloc, {vec.x, vec.y, 0.0f, 1.0f});
    }
    aPoints.push(pAlloc, {p2.x, p2.y, 0.0f, 1.0f});

    return aPoints;
}

static void
insertPoints(
    IAllocator* pAlloc,
    VecBase<PointOnCurve>* aPoints,
    const math::V2& p0,
    const math::V2& p1,
    const math::V2& p2,
    int nTimes = 1
)
{
    for (int i = 1; i < nTimes + 1; ++i)
    {
        f32 t = f32(i) / f32(nTimes + 1);

        auto point = math::bezier(p0, p1, p2, t);
        aPoints->push(pAlloc, {
            .pos = point,
            .bOnCurve = true,
            .bEndOfCurve = false
        });
    }
}

static VecBase<PointOnCurve>
makeItCurvy(IAllocator* pAlloc, const VecBase<PointOnCurve>& aNonCurvyPoints, CurveEndIdx* pEndIdxs, u32 nTessellations)
{
    VecBase<PointOnCurve> aNew(pAlloc, aNonCurvyPoints.getSize());
    utils::fill(pEndIdxs->aIdxs, NPOS16, utils::size(pEndIdxs->aIdxs));
    u16 endIdx = 0;

    u32 firstInCurveIdx = 0;
    bool bPrevOnCurve = true;
    for (auto& p : aNonCurvyPoints)
    {
        u32 idx = aNonCurvyPoints.idx(&p);

        if (p.bEndOfCurve)
        {
            if (!aNonCurvyPoints[idx].bOnCurve || !aNonCurvyPoints[firstInCurveIdx].bOnCurve)
            {
                if (nTessellations > 0)
                {
                    math::V2 p0 {aNonCurvyPoints[idx - 1].pos};
                    math::V2 p1 {aNonCurvyPoints[idx - 0].pos};
                    math::V2 p2 {aNonCurvyPoints[firstInCurveIdx].pos};
                    insertPoints(pAlloc, &aNew, p0, p1, p2, nTessellations);
                }
            }
        }

        if (!bPrevOnCurve)
        {
            if (nTessellations > 0)
            {
                math::V2 p0 {aNonCurvyPoints[idx - 2].pos};
                math::V2 p1 {aNonCurvyPoints[idx - 1].pos};
                math::V2 p2 {aNonCurvyPoints[idx - 0].pos};
                insertPoints(pAlloc, &aNew, p0, p1, p2, nTessellations);
            }
        }

        if (p.bOnCurve)
        {
            aNew.push(pAlloc, {
                .pos = p.pos,
                .bOnCurve = p.bOnCurve,
                .bEndOfCurve = false
            });
        }

        if (p.bEndOfCurve)
        {
            aNew.push(pAlloc, {
                .pos = aNonCurvyPoints[firstInCurveIdx].pos,
                .bOnCurve = true,
                .bEndOfCurve = true,
            });

            if (endIdx < 8) pEndIdxs->aIdxs[endIdx++] = aNew.lastI();
            else assert(false && "8 curves max");

            firstInCurveIdx = idx + 1;
            bPrevOnCurve = true;
        }
        else
        {
            bPrevOnCurve = p.bOnCurve;
        }
    }

    return aNew;
}

VecBase<PointOnCurve>
getPointsWithMissingOnCurve(IAllocator* pAlloc, reader::ttf::Glyph* g)
{
    const auto& aGlyphPoints = g->uGlyph.simple.aPoints;
    u32 size = aGlyphPoints.getSize();

    bool bCurrOnCurve = false;
    bool bPrevOnCurve = false;
    u32 firstInCurveIdx = 0;

    VecBase<PointOnCurve> aPoints(pAlloc, size);
    int nOffCurve = 0;
    for (const auto& p : aGlyphPoints)
    {
        const u32 pointIdx = aGlyphPoints.idx(&p);

        f32 x = f32(p.x);
        f32 y = f32(p.y);

        bool bEndOfCurve = false;
        for (auto e : g->uGlyph.simple.aEndPtsOfContours)
            if (e == pointIdx)
                bEndOfCurve = true;

        math::V2 vCurr {x, y};

        bCurrOnCurve = p.bOnCurve;
        defer( bPrevOnCurve = bCurrOnCurve );

        if (!bCurrOnCurve && !bPrevOnCurve)
        {
            /* insert middle point */
            const auto& prev = aPoints.last();
            math::V2 mid = math::lerp(prev.pos, vCurr, 0.5f);

            aPoints.push(pAlloc, {
                .pos = mid,
                .bOnCurve = true,
                .bEndOfCurve = false
            });
        }

        aPoints.push(pAlloc, {
            .pos {x, y},
            .bOnCurve = p.bOnCurve,
            .bEndOfCurve = bEndOfCurve
        });

        if (!bCurrOnCurve && bEndOfCurve)
            assert(aGlyphPoints[firstInCurveIdx].bOnCurve == true);

        if (bEndOfCurve) firstInCurveIdx = pointIdx + 1;
    }

    return aPoints;
}

static void
drawLineH(u8* pBitmap, const u32 width, const u32 height, int x0, int y0, int x1, int y1)
{
    if (x0 > x1)
    {
        utils::swap(&x0, &x1);
        utils::swap(&y0, &y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir = dy < 0 ? -1 : 1;
    dy *= dir;

    TwoDSpan at(pBitmap, width, height);
    if (dx != 0)
    {
        int y = y0;
        int p = 2*dy - dx;
        for (int i = 0; i < dx+1; ++i)
        {
            at[x0 + i, y] = 0xff;
            if (p >= 0)
            {
                y += dir;
                p = p - 2*dx;
            }
            p = p + 2*dy;
        }
    }
}

static void
drawLineV(u8* pBitmap, const u32 width, const u32 height, int x0, int y0, int x1, int y1)
{
    if (y0 > y1)
    {
        utils::swap(&x0, &x1);
        utils::swap(&y0, &y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir = dx < 0 ? -1 : 1;
    dx *= dir;

    TwoDSpan at(pBitmap, width, height);
    if (dy != 0)
    {
        int x = x0;
        int p = 2*dx - dy;
        for (int i = 0; i < dy+1; ++i)
        {
            at[utils::clamp(x, 0, int(width-1)), y0 + i] = 0xff;

            if (p >= 0)
            {
                x += dir;
                p = p - 2*dy;
            }
            p = p + 2*dx;
        }
    }
}

static void
drawLine(u8* pBitmap, const u32 width, const u32 height, int x0, int y0, int x1, int y1)
{
    if (abs(x1 - x0) > abs(y1 - y0))
        drawLineH(pBitmap, width, height, x0, y0, x1, y1);
    else
        drawLineV(pBitmap, width, height, x0, y0, x1, y1);
}

static void
floodFillDFS(TwoDSpan<u8> s, u16 x, u16 y)
{
    if (x >= s.getHeight() || y >= s.getWidth()) return;

    s[x, y] = 0xff;

    if ((x + 1) < s.getHeight() && s[x + 1, y + 0] != 0xff) floodFillDFS(s, u16(x + 1), u16(y + 0));
    if ((y + 1) < s.getWidth()  && s[x + 0, y + 1] != 0xff) floodFillDFS(s, u16(x + 0), u16(y + 1));
    if ((x - 1) < s.getHeight() && s[x - 1, y + 0] != 0xff) floodFillDFS(s, u16(x - 1), u16(y + 0));
    if ((y - 1) < s.getWidth()  && s[x - 0, y - 1] != 0xff) floodFillDFS(s, u16(x - 0), u16(y - 1));
}

/* https://en.wikipedia.org/wiki/Centroid#Of_a_polygon */
/* NOTE: this doesn't actually work for filling shapes like 'U' or 'G' etc... */
static Pair<u16, u16>
polygonCentroid(
    reader::ttf::Glyph* pGlyph,
    const VecBase<PointOnCurve>& aPoints,
    u32 startIdx,
    u32 width,
    u32 height
)
{
    f32 area = 0;
    f32 scx = 0;
    f32 scy = 0;

    for (u32 i = startIdx + 1; i < aPoints.getSize(); ++i)
    {
        auto& pos0 = aPoints[i - 1].pos;
        auto& pos1 = aPoints[i - 0].pos;

        f32 x0 = (pos0.x / pGlyph->xMax) * width;
        f32 y0 = (pos0.y / pGlyph->yMax) * height;
        f32 x1 = (pos1.x / pGlyph->xMax) * width;
        f32 y1 = (pos1.y / pGlyph->yMax) * height;

        x0 = std::round(utils::min(x0, f32(height - 1)));
        y0 = std::round(utils::clamp(y0, 0.0f, f32(width - 1)));
        x1 = std::round(utils::min(x1, f32(height - 1)));
        y1 = std::round(utils::clamp(y1, 0.0f, f32(width - 1)));

        f32 a = x0*y1 - x1*y0;
        area += a;
        scx += (x0 + x1) * a;
        scy += (y0 + y1) * a;

        if (aPoints[i].bEndOfCurve)
            break;
    }

    f32 _Cx = (1.0 / (6.0 * area/2.0)) * (scx);
    f32 _Cy = (1.0 / (6.0 * area/2.0)) * (scy);

    return {u16(std::round(_Cx)), u16(std::round(_Cy))};
}

static int
polygonArea(const VecBase<PointOnCurve>& aPoints, u32 startIdx)
{
    int area = 0;
    for (u32 i = startIdx + 1; i < aPoints.getSize() && !aPoints[i].bEndOfCurve; ++i)
    {
        int x0 = aPoints[i - 1].pos.x;
        int x1 = aPoints[i - 0].pos.x;
        int y0 = aPoints[i - 1].pos.y;
        int y1 = aPoints[i - 0].pos.y;

        area += (x1 - x0) * (y1 + y0);
    }
    return area / 2;
}

static int
polygonWindingOrder(const VecBase<PointOnCurve>& aPoints, u32 startIdx)
{
    if (aPoints.getSize() - startIdx <= 2)
    {
        LOG_FATAL("How can you get an area of a LINE/DOT?\n");
        return 0;
    }

    return polygonArea(aPoints, startIdx);
}

/* https://sharo.dev/post/reading-ttf-files-and-rasterizing-them-using-a-handmade- */
void
TTF::rasterizeGlyph(IAllocator* pAlloc, reader::ttf::Glyph* pGlyph, TwoDSpan<u8> spBitmap)
{
    const auto& aGlyphPoints = pGlyph->uGlyph.simple.aPoints;

    CurveEndIdx endIdxs;
    auto aCurvyPoints = makeItCurvy(
        pAlloc, getPointsWithMissingOnCurve(pAlloc, pGlyph), &endIdxs, 6
    );

    f32 xMax = m_pFont->m_head.xMax;
    f32 xMin = m_pFont->m_head.xMin;
    f32 yMax = m_pFont->m_head.yMax;
    f32 yMin = m_pFont->m_head.yMin;

    Arr<f32, 32> aIntersections {};
    auto& spBM = spBitmap;

    f32 vScale = f32(spBM.getHeight()) / f32(yMax - yMin);
    f32 hScale = f32(spBM.getWidth()) / f32(xMax - xMin);

    for (u32 row = 0; row < spBM.getHeight(); ++row)
    {
        aIntersections.setSize(0);
        const f32 scanline = f32(row);

        for (u32 pointIdx = 1; pointIdx < aCurvyPoints.getSize(); ++pointIdx)
        {
            f32 x0 = (aCurvyPoints[pointIdx - 1].pos.x) * hScale;
            f32 x1 = (aCurvyPoints[pointIdx - 0].pos.x) * hScale;

            f32 y0 = (aCurvyPoints[pointIdx - 1].pos.y - yMin) * vScale;
            f32 y1 = (aCurvyPoints[pointIdx - 0].pos.y - yMin) * vScale;

            if (aCurvyPoints[pointIdx].bEndOfCurve)
                ++pointIdx;

            /* for the intersection all we need is to find what X is when our y = scanline or when y is equal to i of the loop
             *
             * y - y1 = m*(x - x1) sub both sides by m
             * (y - y1)/m = x - x1 add x1 to both sides
             * (y-y1)*1/m + x1 = x m is just the slope of the line so dy/dx and 1/m is dx/dy
             * y in this equation would be the scanline, x1 & y1 */

            f32 biggerY = utils::max(y0, y1);
            f32 smallerY = utils::min(y0, y1);

            if (scanline <= smallerY || scanline > biggerY) continue;

            f32 dx = x1 - x0;
            f32 dy = y1 - y0;

            if (math::eq(dy, 0.0f)) continue;

            f32 intersection = -1.0f;

            if (math::eq(dx, 0.0f))
                intersection = x1;
            else
                intersection = (scanline - y1)*(dx/dy) + x1;

            aIntersections.push(intersection);
        }

        if (aIntersections.getSize() > 1)
        {
            sort::insertion(&aIntersections);

            for (u32 intIdx = 0; intIdx < aIntersections.getSize(); intIdx += 2)
            {
                f32 start = aIntersections[intIdx];
                int startIdx = start;
                f32 startCovered = (startIdx + 1) - start;

                f32 end = aIntersections[intIdx + 1];
                int endIdx = end;
                f32 endCovered = end - endIdx;

                if (startIdx >= 0)
                    spBM[startIdx, row] = 255.0f * startCovered;

                if (startIdx != endIdx)
                    spBM[endIdx, row] = 255.0f * endCovered;

                for (int col = startIdx + 1; col < endIdx; ++col)
                    if (col >= 0)
                        spBM[col, row] = 255;
            }
        }
    }
}

[[nodiscard]]
static VecBase<CharQuad3Pos2UV>
ttfGenStringMesh(
    TTF* s,
    IAllocator* pAlloc,
    const String str,
    const int xOrigin,
    const int yOrigin,
    const f32 zOff
)
{
    auto getUV = [&](int c) -> f32
    {
        return (c*s->m_scale) / (128.0f * s->m_scale);
    };

    const auto& head = s->m_pFont->m_head;

    VecBase<CharQuad3Pos2UV> aQuads(pAlloc, s->m_maxSize);

    f32 xOff = 0.0f;
    f32 yOff = 0.0f;

    for (char ch : str)
    {
        /* FIXME: uv ordering is messed up (but works correctly) */

        /* tr */
        f32 x0 = 0.0f;
        f32 y0 = getUV(ch + 1);

        /* br */
        f32 x3 = 1.0f;
        f32 y3 = getUV(ch + 1);

        /* tl */
        f32 x1 = 0.0f;
        f32 y1 = getUV(ch + 0);

        /* bl */
        f32 x2 = 1.0f;
        f32 y2 = getUV(ch + 0);

        if (ch == '\n')
        {
            xOff = 0.0f;
            yOff -= 2.0f;
            continue;
        }

        aQuads.push(pAlloc, {
             0.0f + xOff + xOrigin,  2.0f + yOff + yOrigin,  zOff,     x0, y0,
             0.0f + xOff + xOrigin,  0.0f + yOff + yOrigin,  zOff,     x1, y1,
             2.0f + xOff + xOrigin,  0.0f + yOff + yOrigin,  zOff,     x2, y2,

             0.0f + xOff + xOrigin,  2.0f + yOff + yOrigin,  zOff,     x0, y0,
             2.0f + xOff + xOrigin,  0.0f + yOff + yOrigin,  zOff,     x2, y2,
             2.0f + xOff + xOrigin,  2.0f + yOff + yOrigin,  zOff,     x3, y3,
        });

        /* TODO: account for aspect ratio */
        xOff += 1.0f;
    }

    return aQuads;
}

void
TTF::rasterizeAscii(reader::ttf::Font* pFont)
{
    const f32 scale = 128.0f;
    const int iScale = std::round(scale);
    m_scale = scale;
    m_maxSize = 100;

    m_pFont = pFont;

    /* width*height*128 */
    m_pBitmap = (u8*)m_pAlloc->zalloc(1, math::sq(iScale) * 128);

    Arena arena(SIZE_8M);
    defer( arena.freeAll() );

    for (int ch = '!'; ch <= '~'; ++ch)
    {
        u8* pTmp = (u8*)arena.zalloc(1, math::sq(iScale));

        auto g = pFont->readGlyph(ch);
        rasterizeGlyph(&arena, &g, TwoDSpan{pTmp, u32(iScale), u32(iScale)});
        memcpy(m_pBitmap + ch*math::sq(iScale), pTmp, iScale * 128);

        arena.reset();
    }

    texture::Img img {};
    img.setMonochrome(m_pBitmap, iScale, iScale * 128);
    m_texId = img.m_id;

    char test[100] {};
    for (int c = '!', i = 0; c <= '~'; ++c, ++i)
    {
        if (i == 40) test[i++] = '\n';
        test[i] = c;
    }

    auto aQuads = ttfGenStringMesh(this, &arena, test, 0, 0, 1.0f);
    m_vboSize = aQuads.getSize() * 6; /* 6 vertices for 1 quad */

    mtx_lock(&gl::g_mtxGlContext);
    app::g_pWindow->bindGlContext();
    defer(
        app::g_pWindow->unbindGlContext();
        mtx_unlock(&gl::g_mtxGlContext);
    );

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    defer( glBindVertexArray(0) );

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_maxSize * sizeof(CharQuad3Pos2UV), aQuads.data(), GL_DYNAMIC_DRAW);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(f32)*5, (void*)0); /* 3 pos 2 uv */
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(f32)*5, (void*)(sizeof(math::V3)));
}

void
TTF::updateText(IAllocator* pAlloc, const String str, f32 x, f32 y, f32 z)
{
    assert(str.getSize() <= m_maxSize);
    if (m_str == str) return;

    m_str = str;
    auto aQuads = ttfGenStringMesh(this, pAlloc, str, x, y, z);
    m_vboSize = aQuads.getSize() * 6; /* 6 vertices for 1 quad */

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, aQuads.getSize() * sizeof(aQuads[0]), aQuads.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
TTF::draw()
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vboSize);
}

} /* namespace text */
