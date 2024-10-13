#pragma once

#include "adt/String.hh"
#include "gl/gl.hh"
#include "adt/math.hh"
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

void BitmapUpdate(Bitmap* s, Allocator* pAlloc, String str, int x, int y);
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
    Allocator* pAlloc {};
    parser::ttf::Font* pFont {};
    u8* pBitmap {};
    String str {};
    u32 maxSize {};
    f32 scale {};
    GLuint vao {};
    GLuint vbo {};
    GLuint vboSize {};
    GLuint texId {};

    TTF() = default;
    TTF(Allocator* p) : pAlloc(p) {}
};

void TTFRasterizeAsciiTEST(TTF* s, parser::ttf::Font* pFont);
void TTFUpdateText(TTF* s, Allocator* pAlloc, const String str, const int x, const int y, const f32 z);
void TTFDrawAscii(TTF* s);

} /* namespace text */
