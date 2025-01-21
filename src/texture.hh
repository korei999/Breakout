#pragma once

#include "adt/IAllocator.hh"
#include "adt/Map.hh"
#include "adt/Pool.hh"
#include "adt/Vec.hh"
#include "adt/String.hh"
#include "gl/gl.hh" /* IWYU pragma: keep */
#include "adt/math.hh"

namespace texture
{

using namespace adt;

constexpr u32 MAX_COUNT = 256;

struct Img;
struct Hash;

extern Pool<Img, MAX_COUNT> g_aAllTextures;
extern Map<String, PoolHnd> g_mAllTexturesIdxs;

enum TYPE : s8
{
    DIFFUSE = 0,
    NORMAL
};

struct Data
{
    Vec<u8> aData;
    u32 width;
    u32 height;
    u16 bitDepth;
    GLint format;
};

struct Img
{
    IAllocator* m_pAlloc;
    String m_texPath;
    u32 m_width = 0;
    u32 m_height = 0;
    GLuint m_id = 0;
    enum TYPE m_eType = TYPE::DIFFUSE;

    /* */

    Img() = default;
    Img(IAllocator* p) : m_pAlloc(p) {}

    /* */

    void bind(GLint glTex);

    void load(String path, bool bFlip, TYPE type, GLint texMode, GLint magFilter = GL_NEAREST, GLint minFilter = GL_NEAREST_MIPMAP_NEAREST);

    void set(u8* pData, GLint texMode, GLint format, GLsizei width, GLsizei height, GLint magFilter, GLint minFilter);

    void setMonochrome(u8* pData, u32 width, u32 height);

    void destroy();
};

inline void
ImgBind(GLuint id, GLint glTex)
{
    glActiveTexture(glTex);
    glBindTexture(GL_TEXTURE_2D, id);
}

struct ImgLoadArg
{
    Img* self;
    String path;
    bool flip = false;
    TYPE type = TYPE::DIFFUSE;
    GLint texMode = GL_CLAMP_TO_EDGE;
    GLint magFilter = GL_NEAREST;
    GLint minFilter = GL_NEAREST_MIPMAP_NEAREST;
};

struct Framebuffer
{
    GLuint fbo;
    GLuint tex;
    int width;
    int height;
};

struct ShadowMap
{
    Framebuffer t;
};

struct CubeMap
{
    Framebuffer t;
};

struct CubeMapProjections
{
    math::M4 tms[6];

    CubeMapProjections(const math::M4& projection, const math::V3& position);

    math::M4& operator[](size_t i) { assert(i < 6); return tms[i]; }
};

Framebuffer FramebufferCreate(const GLsizei width, const GLsizei height);

ShadowMap ShadowMapCreate(const int width, const int height);

CubeMap CubeMapShadowMapCreate(const int width, const int height);
CubeMap skyBoxCreate(String sFaces[6]);

Data loadBMP(IAllocator* pAlloc, String path, bool flip);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);

inline THREAD_STATUS
ImgSubmit(void* p)
{
    auto a = *(ImgLoadArg*)p;
    a.self->load(a.path, a.flip, a.type, a.texMode, a.magFilter, a.minFilter);
    return {};
}

} /* namespace texure */
