#pragma once

#include "gl/gl.hh"
#include "adt/String.hh"

using namespace adt;

struct TextCharQuad
{
    f32 vs[24];
};

struct Text
{
    String str;
    u32 maxSize;
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;

    Text() = default;
    Text(String s, u32 size, int x, int y, GLint drawMode);
};

void TextUpdate(Text* s, Allocator* pAlloc, String str, int x, int y);
void TextDraw(Text* s);
