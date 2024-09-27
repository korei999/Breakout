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

    VecBase<Points> aPoints(&al.base, 3);
    VecPush(&aPoints, &al.base, {p0.x, p0.y, 0.0f, 1.0f});

    /* quadratic bezier */
    /*B(t) = (1-t)^2*P0 + 2(1-t)*t*P1 + t^2*P2*/

    f32 stepPerIter = 1.0f/nSteps;
    for (int i = 0; i <= nSteps; i++) {
        f32 t = i*stepPerIter;

        auto vec = math::sq(1-t)*p0
            + 2*(1-t)*t*p1
            + math::sq(t)*p2;

        /*COUT("p0: {}, {}\nx: {}, y: {}\n", p0.x, p0.y, x, y);*/

        VecPush(&aPoints, &al.base, {vec.x, vec.y, 0.0f, 1.0f});
    }

    /*VecPush(&aPoints, &al.base, {p1.x, p1.y, 0.0f, 1.0f});*/
    VecPush(&aPoints, &al.base, {p2.x, p2.y, 0.0f, 1.0f});

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

void
TTFGenMesh(TTF* s, const parser::ttf::Glyph& g)
{
    Arena alloc(SIZE_1K);
    defer(ArenaFreeAll(&alloc));

    u32 size = VecSize(&g.uGlyph.simple.aPoints);
    s->maxSize = size;

    VecBase<Points> aQuads(&alloc.base, size);
    for (auto& p : g.uGlyph.simple.aPoints)
    {
        f32 x = (f32(p.x) / f32(g.xMax));
        f32 y = (f32(p.y) / f32(g.yMax));
        VecPush(&aQuads, &alloc.base, {
            x, y, 0.0f, 1.0f
        });
    }

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);
    defer(glBindVertexArray(0));

    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Points) * size,
        VecData(&aQuads),
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

void
TTFDrawOutline(TTF* s)
{
    glBindVertexArray(s->vao);
    glDrawArrays(GL_LINE_STRIP, 0, s->maxSize);
}

} /* namespace text */
