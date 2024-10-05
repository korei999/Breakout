#include "text.hh"

#include "adt/Arena.hh"
#include "adt/Vec.hh"
#include "frame.hh"
#include "adt/defer.hh"

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
makeItCurvy(Allocator* pAlloc, const VecBase<PointOnCurve>& aNonCurvyPoints, CurveEndIdx* pEndIdxs)
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
                math::V2 p0 {aNonCurvyPoints[idx - 1].pos};
                math::V2 p1 {aNonCurvyPoints[idx - 0].pos};
                math::V2 p2 {aNonCurvyPoints[firstInCurveIdx].pos};
                insertPoints(pAlloc, &aNew, p0, p1, p2, 1);
            }
        }
        if (!bPrevOnCurve)
        {
            math::V2 p0 {aNonCurvyPoints[idx - 2].pos};
            math::V2 p1 {aNonCurvyPoints[idx - 1].pos};
            math::V2 p2 {aNonCurvyPoints[idx - 0].pos};
            insertPoints(pAlloc, &aNew, p0, p1, p2, 3);
        }

        if (p.bOnCurve)
        {
            VecPush(&aNew, pAlloc, {
                .pos = p.pos,
                .bOnCurve = p.bOnCurve,
                .bEndOfCurve = p.bEndOfCurve
            });
        }

        if (p.bEndOfCurve)
        {
            /*VecPush(&aNew, pAlloc, {*/
            /*    .pos = aNonCurvyPoints[firstInCurveIdx].pos,*/
            /*    .bOnCurve = true,*/
            /*    .bEndOfCurve = false,*/
            /*});*/

            pEndIdxs->aIdxs[endIdx++] = VecLastI(&aNew);

            firstInCurveIdx = idx + 1;
        }

        if (p.bEndOfCurve) bPrevOnCurve = true;
        else bPrevOnCurve = p.bOnCurve;
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

        f32 x = f32(p.x) / f32(g->xMax);
        f32 y = f32(p.y) / f32(g->yMax);

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

    bool bCurrOnCurve = false;
    bool bPrevOnCurve = false;
    u32 firstInCurveIdx = 0;

    CurveEndIdx endIdxs;
    VecBase<PointOnCurve> aPoints = getPointsWithMissingOnCurve(&alloc.base, g);
    auto aCurvyPoints = makeItCurvy(&alloc.base, aPoints, &endIdxs);

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

} /* namespace text */
