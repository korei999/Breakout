#pragma once

#include "adt/Span2D.hh"
#include "adt/String.hh"
#include "adt/Thread.hh"
#include "adt/math.hh"
#include "gl/gl.hh" /* IWYU pragma: keep */
#include "reader/ttf.hh"

using namespace adt;

namespace text
{

struct Bitmap
{
    String m_str;
    u32 m_maxSize;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_vboSize;

    Bitmap() = default;
    Bitmap(String s, u64 size, int x, int y, GLint drawMode);

    void update(IAllocator* pAlloc, String str, int x, int y);
    void draw();
};

struct CurveEndIdx
{
    u16 aIdxs[8];
};

struct CharToUV
{
    char c {};
    math::V2 texCoords {};
};

struct TTF
{
    IAllocator* m_pAlloc {};
    reader::ttf::Font* m_pFont {};
    String m_str {};
    u8* m_pBitmap {};
    u32 m_maxSize {};
    f32 m_scale {};
    GLuint m_vao {};
    GLuint m_vbo {};
    GLuint m_vboSize {};
    GLuint m_texId {};

    /* */

    TTF() = default;
    TTF(IAllocator* p) : m_pAlloc(p) {}

    /* */

    void rasterizeAscii(reader::ttf::Font* pFont);

    /* xy [0, 0] is bottom left */
    void updateText(IAllocator* pAlloc, const String str, f32 x, f32 y, f32 z);

    void draw();

    /* */

private:
    void rasterizeGlyph(IAllocator* pAlloc, reader::ttf::Glyph* pGlyph, Span2D<u8> spBitmap);
};

struct TTFRasterizeArg
{
    TTF* self {};
    reader::ttf::Font* pFont {};
};

inline THREAD_STATUS
TTFRasterizeSubmit(void* pArg)
{
    auto arg = *(TTFRasterizeArg*)pArg;
    arg.self->rasterizeAscii(arg.pFont);

    return {};
}

} /* namespace text */
