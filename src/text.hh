#pragma once

#include "gl/gl.hh"
#include "adt/String.hh"

using namespace adt;

namespace text
{

struct BitmapCharQuad
{
    f32 vs[24];
};

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

void TextUpdate(Bitmap* s, Allocator* pAlloc, String str, int x, int y);
void TextDraw(Bitmap* s);

} /* namespace text */
