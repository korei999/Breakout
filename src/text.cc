#include "text.hh"

#include "adt/Arena.hh"
#include "adt/Vec.hh"
#include "adt/defer.hh"
#include "frame.hh"

namespace text
{

struct CharQuad
{
    f32 vs[24]; /* 6(2 triangles) * 4(2 pos, 2 uv coords) */
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

static VecBase<CharQuad> TextUpdateBuffer(Bitmap* s, Allocator* pAlloc, String str, u32 size, int xOrigin, int yOrigin);
static void TextGenMesh(Bitmap* s, int xOrigin, int yOrigin, GLint drawMode);

Bitmap::Bitmap(String s, u64 size, int x, int y, GLint drawMode)
    : str(s), maxSize(size)
{
    TextGenMesh(this, x, y, drawMode);
}

static void
TextGenMesh(Bitmap* s, int xOrigin, int yOrigin, GLint drawMode)
{
    Arena allocScope(SIZE_1M);
    defer(ArenaFreeAll(&allocScope));

    auto aQuads = TextUpdateBuffer(s, &allocScope.base, s->str, s->maxSize, xOrigin, yOrigin);

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);

    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glBufferData(GL_ARRAY_BUFFER, s->maxSize * sizeof(CharQuad), VecData(&aQuads), drawMode);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(math::V2), (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(math::V2), (void*)(sizeof(math::V2)));

    glBindVertexArray(0);
}

static VecBase<CharQuad>
TextUpdateBuffer(Bitmap* s, Allocator* pAlloc, String str, u32 size, int xOrigin, int yOrigin)
{
    VecBase<CharQuad> aQuads(pAlloc, size);
    memset(VecData(&aQuads), 0, sizeof(CharQuad) * size);

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

        VecPush(&aQuads, pAlloc, {
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

    s->vboSize = VecSize(&aQuads) * 6; /* 6 vertices for 1 quad */

    return aQuads;
}

void
BitmapUpdate(Bitmap* s, Allocator* pAlloc, String str, int x, int y)
{
    assert(str.size <= s->maxSize);

    s->str = str;
    VecBase<CharQuad> aQuads = TextUpdateBuffer(s, pAlloc, str, s->maxSize, x, y);

    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, s->maxSize * sizeof(f32) * 4 * 6, VecData(&aQuads));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
BitmapDraw(Bitmap* s)
{
    glBindVertexArray(s->vao);
    glDrawArrays(GL_TRIANGLES, 0, s->vboSize);
}

static VecBase<Point>
getBeizerPoints(
    Allocator* pAlloc,
    const math::V2& p0,
    const math::V2& p1,
    const math::V2& p2,
    int nSteps)
{
    /* quadratic bezier */
    /*B(t) = (1-t)^2*P0 + 2(1-t)*t*P1 + t^2*P2*/

    VecBase<Point> aPoints(pAlloc, 2 + nSteps);
    VecPush(&aPoints, pAlloc, {p0.x, p0.y, 0.0f, 1.0f});
    for (int i = 1; i < nSteps; i++) {
        f32 t = f32(i) / f32(nSteps);

        math::V2 vec = math::bezier(p0, p1, p2, t);

        VecPush(&aPoints, pAlloc, {vec.x, vec.y, 0.0f, 1.0f});
    }
    VecPush(&aPoints, pAlloc, {p2.x, p2.y, 0.0f, 1.0f});

    return aPoints;
}

void
TTFGenBezierMesh(TTF* s, const math::V2& p0, const math::V2& p1, const math::V2& p2, int nSteps)
{
    Arena al(SIZE_1K);
    defer(ArenaFreeAll(&al));

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);
    defer(glBindVertexArray(0));

    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);

    VecBase<Point> aPoints = getBeizerPoints(&al.base, p0, p1, p2, nSteps);
    s->maxSize = VecSize(&aPoints);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Point) * VecSize(&aPoints),
        VecData(&aPoints),
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(f32), (void*)0
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(f32), (void*)(sizeof(f32) * 2)
    );
}

static void
insertPoints(
    Allocator* pAlloc,
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
        VecPush(aPoints, pAlloc, {
            .pos = point,
            .bOnCurve = true,
            .bEndOfCurve = false
        });
    }
}

static VecBase<PointOnCurve>
makeItCurvy(Allocator* pAlloc, const VecBase<PointOnCurve>& aNonCurvyPoints, CurveEndIdx* pEndIdxs, bool bTessellate = true)
{
    VecBase<PointOnCurve> aNew(pAlloc, VecSize(&aNonCurvyPoints));
    utils::fill(pEndIdxs->aIdxs, NPOS16, utils::size(pEndIdxs->aIdxs));
    u16 endIdx = 0;

    u32 firstInCurveIdx = 0;
    bool bPrevOnCurve = true;
    for (auto& p : aNonCurvyPoints)
    {
        u32 idx = VecIdx(&aNonCurvyPoints, &p);

        if (p.bEndOfCurve)
        {
            if (!aNonCurvyPoints[idx].bOnCurve || !aNonCurvyPoints[firstInCurveIdx].bOnCurve)
            {
                if (bTessellate)
                {
                    math::V2 p0 {aNonCurvyPoints[idx - 1].pos};
                    math::V2 p1 {aNonCurvyPoints[idx - 0].pos};
                    math::V2 p2 {aNonCurvyPoints[firstInCurveIdx].pos};
                    insertPoints(pAlloc, &aNew, p0, p1, p2, 6);
                }
            }
        }

        if (!bPrevOnCurve)
        {
            if (bTessellate)
            {
                math::V2 p0 {aNonCurvyPoints[idx - 2].pos};
                math::V2 p1 {aNonCurvyPoints[idx - 1].pos};
                math::V2 p2 {aNonCurvyPoints[idx - 0].pos};
                insertPoints(pAlloc, &aNew, p0, p1, p2, 6);
            }
        }

        if (p.bOnCurve)
        {
            VecPush(&aNew, pAlloc, {
                .pos = p.pos,
                .bOnCurve = p.bOnCurve,
                .bEndOfCurve = false
            });
        }

        if (p.bEndOfCurve)
        {
            VecPush(&aNew, pAlloc, {
                .pos = aNonCurvyPoints[firstInCurveIdx].pos,
                .bOnCurve = true,
                .bEndOfCurve = true,
            });

            if (endIdx < 8) pEndIdxs->aIdxs[endIdx++] = VecLastI(&aNew);
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
getPointsWithMissingOnCurve(Allocator* pAlloc, parser::ttf::Glyph* g)
{
    const auto& aGlyphPoints = g->uGlyph.simple.aPoints;
    u32 size = VecSize(&aGlyphPoints);

    LOG("numberOfContours: {}\n", g->numberOfContours);

    bool bCurrOnCurve = false;
    bool bPrevOnCurve = false;
    u32 firstInCurveIdx = 0;

    VecBase<PointOnCurve> aPoints(pAlloc, size);
    int nOffCurve = 0;
    for (const auto& p : aGlyphPoints)
    {
        const u32 pointIdx = VecIdx(&aGlyphPoints, &p);

        /*f32 x = f32(p.x) / f32(g->xMax);*/
        /*f32 y = f32(p.y) / f32(g->yMax);*/

        f32 x = f32(p.x);
        f32 y = f32(p.y);

        bool bEndOfCurve = false;
        for (auto e : g->uGlyph.simple.aEndPtsOfContours)
            if (e == pointIdx)
                bEndOfCurve = true;

        math::V2 vCurr {x, y};

        bCurrOnCurve = p.bOnCurve;
        defer(bPrevOnCurve = bCurrOnCurve);

        if (!bCurrOnCurve && !bPrevOnCurve)
        {
            /* insert middle point */
            const auto& prev = VecLast(&aPoints);
            math::V2 mid = math::lerp(prev.pos, vCurr, 0.5f);

            VecPush(&aPoints, pAlloc, {
                .pos = mid,
                .bOnCurve = true,
                .bEndOfCurve = false
            });
        }

        VecPush(&aPoints, pAlloc, {
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

CurveEndIdx
TTFGenMesh(TTF* s, parser::ttf::Glyph* g)
{
    Arena alloc(SIZE_8K * 2);
    defer(ArenaFreeAll(&alloc));

    const auto& aGlyphPoints = g->uGlyph.simple.aPoints;
    u32 size = VecSize(&aGlyphPoints);

    CurveEndIdx endIdxs;
    VecBase<PointOnCurve> aPoints = getPointsWithMissingOnCurve(&alloc.base, g);
    auto aCurvyPoints = makeItCurvy(&alloc.base, aPoints, &endIdxs, true);

    s->maxSize = VecSize(&aCurvyPoints);

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);
    defer(glBindVertexArray(0));

    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(*VecData(&aCurvyPoints)) * VecSize(&aCurvyPoints),
        VecData(&aCurvyPoints),
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(f32), (void*)0
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(f32), (void*)(sizeof(f32) * 2)
    );

    return endIdxs;
}

static void
TTFDrawLinesOrDots(TTF* s, GLint drawMode, u32 max)
{
    glBindVertexArray(s->vao);
    glDrawArrays(drawMode, 0, max);
}

void
TTFDrawOutline(TTF* s, u32 max)
{
    TTFDrawLinesOrDots(s, GL_LINE_LOOP, max == 0 ? s->maxSize : max);
}

void
TTFDrawDots(TTF* s, u32 max)
{
    TTFDrawLinesOrDots(s, GL_POINTS, max == 0 ? s->maxSize : max);
}

void
TTFDrawCorrectLines(TTF* s, const CurveEndIdx& ends)
{
    glBindVertexArray(s->vao);

    u32 off = 0;
    u32 endOff = 1;
    const auto& g = ends.aIdxs;

    for (u32 i = 0; i < 8 && ends.aIdxs[i] != NPOS16; i++)
    {
        auto ecIdx = g[i] + endOff;
        glDrawArrays(GL_LINE_STRIP, off, ecIdx + 1 - off);
        off = ecIdx + 1;
        ++endOff;
    }
}

#define AT(x, y) pBitmap[width*int(x) + int(y)]

static void
drawLineH(
    u8* pBitmap,
    const u32 width,
    const u32 height,
    int x0,
    int y0,
    int x1,
    int y1
)
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

    if (dx != 0)
    {
        int y = y0;
        int p = 2*dy - dx;
        for (int i = 0; i < dx+1; ++i)
        {
            AT(y, x0 + i) = 0xff;
            /*AT(x0 + i, y) = 0xff;*/
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
drawLineV(
    u8* pBitmap,
    const u32 width,
    const u32 height,
    int x0,
    int y0,
    int x1,
    int y1
)
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

    if (dy != 0)
    {
        int x = x0;
        int p = 2*dx - dy;
        for (int i = 0; i < dy+1; ++i)
        {
            /*AT(utils::clamp(x, 0, int(height-1)), y0 + i) = 0xff;*/
            AT(y0 + i, utils::clamp(x, 0, int(width-1))) = 0xff;
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
drawLine(
    u8* pBitmap,
    const u32 width,
    const u32 height,
    int x0,
    int y0,
    int x1,
    int y1
)
{
    if (abs(x1 - x0) > abs(y1 - y0))
        drawLineH(pBitmap, width, height, x0, y0, x1, y1);
    else
        drawLineV(pBitmap, width, height, x0, y0, x1, y1);
}

static void
scanLineThing(Allocator* pAlloc, u8* pBitmap, const u32 width, const u32 height)
{
    Vec<u16> aIntersections(pAlloc, 50);

    for (u16 i = 0; i < height; ++i)
    {
        VecSetSize(&aIntersections, 0);
        for (u16 j = 0; j < width; ++j)
        {
            /* find all the white point on the line */
            /*if (AT(i, j) == 0xff)*/
            /*    VecPush(&aIntersections, j);*/

            /* TODO: actually just push lastest consecutive white */

            if (AT(i, j) == 0xff)
            {
                while (AT(i, j) == 0xff && j < width)
                    ++j;
                --j;

                VecPush(&aIntersections, j);
            }
        }

        COUT("i({}, size: {}): ", i, VecSize(&aIntersections));
        for (auto ip : aIntersections)
            COUT("{}, ", ip);
        COUT("\n");
        
        if (VecSize(&aIntersections) > 1)
        {
            /* treat one consetive row of white points as one point */
            for (u16 it = 1; it < VecSize(&aIntersections); ++it)
            {

            }
        }
    }
}

enum class FLOOD_FILL_MODE : u8
{
    XOR = 0, OR
};

static void
floodFillBFS(u8* pBitmap, const u32 width, const u32 height, Pair<u16, u16> pos)
{
    AT(pos.x, pos.y) = 0xff;

    if (AT(pos.x + 1, pos.y + 0) != 0xff) floodFillBFS(pBitmap, width, height, {pos.x + 1, pos.y + 0});
    if (AT(pos.x + 0, pos.y + 1) != 0xff) floodFillBFS(pBitmap, width, height, {pos.x + 0, pos.y + 1});
    if (AT(pos.x - 1, pos.y + 0) != 0xff) floodFillBFS(pBitmap, width, height, {pos.x - 1, pos.y + 0});
    if (AT(pos.x - 0, pos.y - 1) != 0xff) floodFillBFS(pBitmap, width, height, {pos.x - 0, pos.y - 1});
}

static void
floodFillThing(Allocator* pAlloc, u8* pBitmap, const u32 width, const u32 height, FLOOD_FILL_MODE eMode)
{
    Pair<u16, u16> picked = {NPOS16, NPOS16};
    for (u32 i = 0; i < height; ++i)
    {
        for (u32 j = 0; j < width; ++j)
        {
            if (AT(i, j) == 0xff)
            {
                /* NOTE: stupid heuristic */
                u32 off = 0;
                while (AT(i, j + off) == 0xff)
                    ++off;

                picked = {i + 1, (j + j+off) / 2};

                goto skip;
            }
        }
    }

skip:

    if (picked == Pair{NPOS16, NPOS16} && picked < Pair{u16(width), u16(height)})
        assert(false && "no white points?");
    else
        floodFillBFS(pBitmap, width, height, picked);
}

/* TODO: rasterize string later */
u8*
TTFRasterizeTEST(TTF* s, parser::ttf::Glyph* pGlyph, u32 width, u32 height)
{
    Arena arena(SIZE_8K);
    defer(ArenaFreeAll(&arena));

    const auto& aGlyphPoints = pGlyph->uGlyph.simple.aPoints;
    u32 size = VecSize(&aGlyphPoints);

    CurveEndIdx endIdxs;

    VecBase<PointOnCurve> aPoints = getPointsWithMissingOnCurve(&arena.base, pGlyph);
    auto aCurvyPoints = makeItCurvy(&arena.base, aPoints, &endIdxs, true);
    /*auto& aCurvyPoints = aPoints;*/

    u8* pBitmap = (u8*)::calloc(1, width*height);

    auto AT = [&](int x, int y) -> u8& {
        assert(u32(x) < width && u32(y) < height);
        return pBitmap[width*x + y];
    };

    COUT("xMin: {}, xMax: {}, yMin: {}, yMax: {}\n", pGlyph->xMin, pGlyph->xMax, pGlyph->yMin, pGlyph->yMax);

    bool bPrevWasEnd = false;
    int cyPrev = 0;
    int cxPrev = 0;
    for (const auto& p : aCurvyPoints)
    {
        const u32 idx = VecIdx(&aCurvyPoints, &p);
        const auto& pos = p.pos;

        f32 x = (pos.x / pGlyph->xMax) * width;
        f32 y = (pos.y / pGlyph->yMax) * height;

        int cy = roundf(utils::clamp(y, 0.0f, f32(width - 1)));
        int cx = roundf(utils::min(x, f32(height - 1)));

        if (idx != 0 && !bPrevWasEnd)
        {
            /*AT(cy, cx) = 0xff;*/
            drawLine(pBitmap, width, height, cxPrev, cyPrev, cx, cy);
        }

        if (p.bEndOfCurve)
            bPrevWasEnd = true;
        else bPrevWasEnd = false;

        /*if (p.bEndOfCurve)*/
        /*    break;*/

        cyPrev = cy;
        cxPrev = cx;
    }

    floodFillThing(&arena.base, pBitmap, width, height, FLOOD_FILL_MODE::XOR);

    return pBitmap;
}

#undef AT

} /* namespace text */
