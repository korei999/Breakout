#pragma once

#include "adt/Allocator.hh"
#include "adt/HashMap.hh"
#include "adt/Vec.hh"
#include "adt/String.hh"
#include "gl/gl.hh"
#include "math.hh"

struct Texture;
struct TextureHash;

extern Vec<Texture> g_aAllTextures;
extern HashMap<TextureHash> g_mAllTexturesIdxs;

struct TextureHash
{
    String sPathKey {};
    GLuint vecIdx {};
};

inline bool
operator==(const TextureHash& l, const TextureHash& r)
{
    return l.sPathKey == r.sPathKey;
}

template<>
inline u64
hash::func(const TextureHash& x)
{
    return hash::func(x.sPathKey);
}

enum TEX_TYPE : s8
{
    DIFFUSE = 0,
    NORMAL
};

struct TextureData
{
    Vec<u8> aData;
    u32 width;
    u32 height;
    u16 bitDepth;
    GLint format;
};

struct Texture
{
    Allocator* pAlloc;
    String texPath;
    u32 width;
    u32 height;
    GLuint id = 0;
    enum TEX_TYPE type;

    Texture() = default;
    Texture(Allocator* p) : pAlloc(p) {}
};

struct TexLoadArg
{
    Texture* self;
    String path;
    TEX_TYPE type;
    bool flip;
    GLint texMode;
    GLint magFilter;
    GLint minFilter;
};

struct TextureFramebuffer
{
    GLuint fbo;
    GLuint tex;
    int width;
    int height;
};

struct ShadowMap
{
    TextureFramebuffer t;
};

struct CubeMap
{
    TextureFramebuffer t;
};

struct CubeMapProjections
{
    math::M4 tms[6];

    CubeMapProjections(const math::M4& projection, const math::V3& position);

    math::M4& operator[](size_t i) { return tms[i]; }
};

void TextureBind(Texture* s, GLint glTex);
void TextureBind(GLuint id, GLint glTex);
void TextureLoad(Texture* s, String path, TEX_TYPE type, bool flip, GLint texMode, GLint magFilter = GL_NEAREST, GLint minFilter = GL_NEAREST_MIPMAP_NEAREST);
void TextureDestroy(Texture* s);
TextureFramebuffer TexFramebufferCreate(const GLsizei width, const GLsizei height);
ShadowMap ShadowMapCreate(const int width, const int height);
CubeMap CubeShadowMapCreate(const int width, const int height);
CubeMap SkyBoxCreate(String sFaces[6]);
TextureData loadBMP(Allocator* pAlloc, String path, bool flip);
void flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip);
void flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip);

inline int
TextureSubmit(void* p)
{
    auto a = *(TexLoadArg*)p;
    TextureLoad(a.self, a.path, a.type, a.flip, a.texMode, a.magFilter, a.minFilter);
    return 0;
}
