#pragma once

#include "adt/Allocator.hh"
#include "adt/Map.hh"
#include "adt/Vec.hh"
#include "adt/String.hh"
#include "gl/gl.hh"
#include "adt/math.hh"

namespace texture
{

using namespace adt;

struct Img;
struct Hash;

extern Vec<Img> g_aAllTextures;
extern Map<Hash> g_mAllTexturesIdxs;

struct Hash
{
    String sPathKey {};
    u32 vecIdx {};
};

inline bool
operator==(const Hash& l, const Hash& r)
{
    return l.sPathKey == r.sPathKey;
}

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
    Allocator* pAlloc;
    String texPath;
    u32 width = 0;
    u32 height = 0;
    GLuint id = 0;
    enum TYPE type = TYPE::DIFFUSE;

    Img() = default;
    Img(Allocator* p) : pAlloc(p) {}
};

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

void ImgBind(Img* s, GLint glTex);
void ImgBind(GLuint id, GLint glTex);

void
ImgLoad(
    Img* s,
    String path,
    bool bFlip,
    TYPE type,
    GLint texMode,
    GLint magFilter = GL_NEAREST,
    GLint minFilter = GL_NEAREST_MIPMAP_NEAREST
);

void 
ImgSet(
    Img* s,
    u8* pData,
    GLint texMode,
    GLint format,
    GLsizei width,
    GLsizei height,
    GLint magFilter,
    GLint minFilter
);

void ImgSetMonochrome(Img* s, u8* pData, u32 width, u32 height);
void ImgDestroy(Img* s);

Framebuffer FramebufferCreate(const GLsizei width, const GLsizei height);

ShadowMap ShadowMapCreate(const int width, const int height);

CubeMap CubeMapShadowMapCreate(const int width, const int height);

CubeMap skyBoxCreate(String sFaces[6]);

Data loadBMP(Allocator* pAlloc, String path, bool flip);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);

inline int
ImgSubmit(void* p)
{
    auto a = *(ImgLoadArg*)p;
    ImgLoad(a.self, a.path, a.flip, a.type, a.texMode, a.magFilter, a.minFilter);
    return 0;
}

} /* namespace texure */

template<>
inline adt::u64
adt::hash::func(const texture::Hash& x)
{
    return hash::func(x.sPathKey);
}
