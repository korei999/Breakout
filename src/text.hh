#pragma once

#include "adt/String.hh"
#include "gl/gl.hh"
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

struct TTF
{
    String str;
    u32 maxSize;
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;
};

void TTFGenMesh(TTF* s, const parser::ttf::Glyph& g);
void TTFDrawOutline(TTF* s);

} /* namespace text */
