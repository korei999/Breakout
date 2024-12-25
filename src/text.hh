#pragma once

#include "adt/String.hh"
#include "adt/math.hh"
#include "gl/gl.hh" /* IWYU pragma: keep */
#include "parser/ttf.hh"

using namespace adt;

namespace text
{

struct Bitmap
{
    String str;
    u32 maxSize;
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;

    Bitmap() = default;
    Bitmap(String s, u64 size, int x, int y, GLint drawMode);
};

void BitmapUpdate(Bitmap* s, IAllocator* pAlloc, String str, int x, int y);
void BitmapDraw(Bitmap* s);

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
    parser::ttf::Font* m_pFont {};
    u8* m_pBitmap {};
    String m_str {};
    u32 m_maxSize {};
    f32 m_scale {};
    GLuint m_vao {};
    GLuint m_vbo {};
    GLuint m_vboSize {};
    GLuint m_texId {};

    TTF() = default;
    TTF(IAllocator* p) : m_pAlloc(p) {}

    void rasterizeAscii(parser::ttf::Font* pFont);
    void updateText(IAllocator* pAlloc, const String str, const int x, const int y, const f32 z);
    void draw();
};

struct TTFRasterizeArg
{
    TTF* self {};
    parser::ttf::Font* pFont {};
};

inline int
TTFRasterizeSubmit(void* pArg)
{
    auto arg = *(TTFRasterizeArg*)pArg;
    arg.self->rasterizeAscii(arg.pFont);

    return thrd_success;
}

} /* namespace text */
