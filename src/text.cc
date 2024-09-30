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

struct Points
{
    f32 x, y, u, v;
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

static VecBase<Points>
getBeizerPoints(
    Allocator* pAlloc,
    const math::V2& p0,
    const math::V2& p1,
    const math::V2& p2,
    int nSteps)
{
    /* quadratic bezier */
    /*B(t) = (1-t)^2*P0 + 2(1-t)*t*P1 + t^2*P2*/

    VecBase<Points> aPoints(pAlloc, 2 + nSteps);
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

    VecBase<Points> aPoints = getBeizerPoints(&al.base, p0, p1, p2, nSteps);
    s->maxSize = VecSize(&aPoints);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Points) * VecSize(&aPoints),
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
addBunchOfDots(const VecBase<Points>& s, VecBase<Points>* a, Allocator* p, int i, int nDegree)
{
    const auto& itPoint = VecLast(&s);

    int nSteps = 10;
    switch (nDegree)
    {
        default: assert(false);

        case 0: break;

        case 1: {
            for (int n = 1; n < nSteps; ++n)
            {
                f32 t = f32(n) / f32(nSteps);

                math::V2 vpp {s[i - 2].x, s[i - 2].y};
                math::V2 vp  {s[i - 1].x, s[i - 1].y};
                math::V2 vc  {itPoint.x, itPoint.y};

                math::V2 dot = math::bezier(vpp, vp, vc, t);
                VecPush(a, p, {dot.x, dot.y, 1.0f, 0.0f});
            }
        } break;

        case 2: {
            for (int n = 1; n < nSteps; ++n)
            {
                f32 t = f32(n) / f32(nSteps);

                math::V2 vppp {s[i - 3].x, s[i - 3].y};
                math::V2 vpp  {s[i - 2].x, s[i - 2].y};
                math::V2 vp   {s[i - 1].x, s[i - 1].y};
                math::V2 vc   {itPoint.x, itPoint.y};

                math::V2 dot = math::bezier(vppp, vpp, vp, vc, t);
                VecPush(a, p, {dot.x, dot.y, 1.0f, 0.0f});
            }
        } break;

        case 3: {
            for (int n = 1; n < nSteps; ++n)
            {
                f32 t = f32(n) / f32(nSteps);

                math::V2 vpppp {s[i - 4].x, s[i - 4].y};
                math::V2 vppp  {s[i - 3].x, s[i - 3].y};
                math::V2 vpp   {s[i - 2].x, s[i - 2].y};
                math::V2 vp    {s[i - 1].x, s[i - 1].y};
                math::V2 vc    {itPoint.x, itPoint.y};

                math::V2 dot = math::bezier(vpppp, vppp, vpp, vp, vc, t);
                VecPush(a, p, {dot.x, dot.y, 1.0f, 0.0f});
            }
        } break;

        case 4: {
            for (int n = 1; n < nSteps; ++n)
            {
                f32 t = f32(n) / f32(nSteps);

                math::V2 vppppp {s[i - 5].x, s[i - 5].y};
                math::V2 vpppp  {s[i - 4].x, s[i - 4].y};
                math::V2 vppp   {s[i - 3].x, s[i - 3].y};
                math::V2 vpp    {s[i - 2].x, s[i - 2].y};
                math::V2 vp     {s[i - 1].x, s[i - 1].y};
                math::V2 vc     {itPoint.x, itPoint.y};

                math::V2 dot = math::bezier(vppppp, vpppp, vppp, vpp, vp, vc, t);
                VecPush(a, p, {dot.x, dot.y, 1.0f, 0.0f});
            }
        } break;
    }
}

static VecBase<Points>
bezieriteArray(const VecBase<Points>& s, Allocator* p, parser::ttf::Glyph* g)
{
    VecBase<Points> a(p, VecSize(&s));

    Arena al(SIZE_1K);
    defer(ArenaFreeAll(&al));
    Vec<Points> aBezPoints(&al.base, 5);

    const auto& aEndsOfContours = g->uGlyph.simple.aEndPtsOfContours;
    const auto& aGlyphPoints = g->uGlyph.simple.aPoints;

    u32 firstInCurveIdx = 0;
    u32 firstInCurveSrcIdx = 0;

    u32 nOffCurve = 0;
    for (u32 i = 0; i < VecSize(&s); ++i)
    {
        const auto& itPoint = VecAt(&s, i);

        bool bEndOfCurve = false;
        for (auto endIdx : g->uGlyph.simple.aEndPtsOfContours)
        {
            /*COUT("i: {}, endIdx: {}\n", i, endIdx);*/
            if (i == endIdx)
            {
                bEndOfCurve = true;
                break;
            }
        }

        if (!aGlyphPoints[i].bOnCurve && !bEndOfCurve)
        {
            /*VecPush(&aBezPoints, s[i]);*/
            ++nOffCurve;
            continue;
        }

        /* bezier through each offCurve point */
        /*COUT("nOffCurve: {}, size: {}\n", nOffCurve, VecSize(&aBezPoints));*/
        /*addBunchOfDots(aBezPoints.base, &a, p, i, nOffCurve);*/

        nOffCurve = 0;

        VecPush(&a, p, VecAt(&s, i));

        VecSetSize(&aBezPoints, 0);
    }

    return a;
}

void
TTFGenMesh(TTF* s, parser::ttf::Glyph* g)
{
    Arena alloc(SIZE_1K);
    defer(ArenaFreeAll(&alloc));

    s->glyph = *g;

    const auto& aGlyphPoints = g->uGlyph.simple.aPoints;
    u32 size = VecSize(&aGlyphPoints);

    [[maybe_unused]] bool bPrevOffCurve = false;

    u32 firstInContourIdx = 0;
    u32 off = 0;
    VecBase<Points> aPoints(&alloc.base, size);
    for (const auto& p : aGlyphPoints)
    {
        const u32 pointIdx = VecIdx(&aGlyphPoints, &p);
        bool bCurrOffCurve = !p.bOnCurve;

        f32 x = f32(p.x) / f32(g->xMax);
        f32 y = f32(p.y) / f32(g->yMax);

        math::V2 vCurr {x, y};

        VecPush(&aPoints, &alloc.base, {
            x, y, 0.0f, 1.0f
        });

        // if (bPrevOffCurve && bCurrOffCurve && (pointIdx + 1) < VecSize(&aGlyphPoints))
        // {
        //     auto& prev = aGlyphPoints[pointIdx - 1];
        //     auto& next = VecAt(&aGlyphPoints, pointIdx + 1);

        //     math::V2 vPrev {f32(prev.x) / f32(g->xMax), f32(prev.y) / f32(g->yMax)};
        //     math::V2 vMid = math::lerp(vPrev, vCurr, 0.5f);
        //     math::V2 vNext {f32(next.x) / f32(g->xMax), f32(next.y) / f32(g->yMax)};

        //     VecPush(&aPoints, &alloc.base, {
        //         vMid.x, vMid.y, 1.0f, 0.0f
        //     });
        //     VecSwapWithLast(&aPoints, VecLastI(&aPoints) - 1);

        //     /*++off;*/
        // }

        // for (auto endContourIdx : g->uGlyph.simple.aEndPtsOfContours)
        // {
        //     if (endContourIdx + off == pointIdx)
        //     {
        //         VecPush(&aPoints, &alloc.base, aPoints[firstInContourIdx]);
        //         firstInContourIdx = VecLastI(&aPoints) + 1;
        //         break;
        //     }
        // }

        bPrevOffCurve = bCurrOffCurve;
    }

    VecBase<Points> aBeziered = bezieriteArray(aPoints, &alloc.base, g);

    /*s->maxSize = VecSize(&aPoints);*/
    s->maxSize = VecSize(&aBeziered);

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);
    defer(glBindVertexArray(0));

    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(*VecData(&aBeziered)) * VecSize(&aBeziered),
        VecData(&aBeziered),
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
TTFDrawLinesOrDots(TTF* s, GLint drawMode, u32 max)
{
    glBindVertexArray(s->vao);
    glDrawArrays(drawMode, 0, max);
}

void
TTFDrawOutline(TTF* s, u32 max)
{
    TTFDrawLinesOrDots(s, GL_LINE_STRIP, max == 0 ? s->maxSize : max);
}

void
TTFDrawDots(TTF* s, u32 max)
{
    TTFDrawLinesOrDots(s, GL_POINTS, max == 0 ? s->maxSize : max);
}

void
TTFDrawCorrectLines(TTF* s)
{
    glBindVertexArray(s->vao);

    auto& g = s->glyph.uGlyph.simple.aEndPtsOfContours;

    u32 off = 0;
    u32 endOff = 1;

    for (u32 i = 0; i < VecSize(&g); i++)
    {
        auto ecIdx = g[i] + endOff;
        glDrawArrays(GL_LINE_STRIP, off, ecIdx + 1 - off);
        off = ecIdx + 1;
        ++endOff;
    }
}

} /* namespace text */
