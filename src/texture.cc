#include "texture.hh"

#include "IWindow.hh"
#include "adt/Arena.hh"
#include "adt/OsAllocator.hh"
#include "adt/logs.hh"
#include "app.hh"
#include "parser/Bin.hh"

#include <immintrin.h>

/* Bitmap file format
 *
 * SECTION
 * Address:Bytes	Name
 *
 * HEADER:
 *	  0:	2		"BM" magic number
 *	  2:	4		file size
 *	  6:	4		junk
 *	 10:	4		Starting address of image data
 * BITMAP HEADER:
 *	 14:	4		header size
 *	 18:	4		width  (signed)
 *	 22:	4		height (signed)
 *	 26:	2		Number of color planes
 *	 28:	2		Bits per pixel
 *	[...]
 * [OPTIONAL COLOR PALETTE, NOT PRESENT IN 32 BIT BITMAPS]
 * BITMAP DATA:
 *	DATA:	X	Pixels
 */

namespace texture
{

Pool<Img, texture::MAX_COUNT> g_aAllTextures(INIT);
Map<String, PoolHnd> g_mAllTexturesIdxs(inl_pOsAlloc, texture::MAX_COUNT);

static mtx_t s_mtxAllTextures;
static once_flag s_onceFlagAllTextures = ONCE_FLAG_INIT;

void
ImgLoad(Img* s, String path, bool bFlip, TYPE type, GLint texMode, GLint magFilter, GLint minFilter)
{
    call_once(&s_onceFlagAllTextures, +[]{
        mtx_init(&s_mtxAllTextures, mtx_plain);
    });

    u32 idx = NPOS;

    {
        guard::Mtx lock(&s_mtxAllTextures);

        auto fTried = MapSearch(&g_mAllTexturesIdxs, {path});
        if (fTried)
        {
            LOG_WARN("duplicate texture: '{}'\n", path);
            return;
        }

        idx = PoolRent(&g_aAllTextures, *s);
        MapInsert(&g_mAllTexturesIdxs, path, idx);
    }

#ifdef D_TEXTURE
    LOG_OK("loading '{}' texture...\n", path);
#endif

    if (s->id != 0) LOG_FATAL("id != 0: '{}'\n", s->id);

    Arena al(SIZE_1M * 5);
    defer( ArenaFreeAll(&al) );
    Data img = loadBMP(&al.super, path, bFlip);

    s->texPath = path;
    s->type = type;

    Vec<u8> pixels = img.aData;

    ImgSet(s, VecData(&pixels), texMode, img.format, img.width, img.height, magFilter, minFilter);

    s->width = img.width;
    s->height = img.height;
    
    auto found = MapSearch(&g_mAllTexturesIdxs, path);
    if (found)
    {
        u32 idx = found.pData->val;
        g_aAllTextures[idx] = *s;
    }
    else LOG_FATAL("Why didn't find?\n");
}

void
ImgDestroy(Img* s)
{
    if (s->id != 0)
    {
        glDeleteTextures(1, &s->id);
        LOG_OK("Texture '{}' destroyed\n", s->id);
        s->id = 0;
    }
}

void
ImgBind(Img* s, GLint glTex)
{
    glActiveTexture(glTex);
    glBindTexture(GL_TEXTURE_2D, s->id);
}

void
ImgBind(GLuint id, GLint glTex)
{
    glActiveTexture(glTex);
    glBindTexture(GL_TEXTURE_2D, id);
}

void
ImgSet(Img* s, u8* pData, GLint texMode, GLint format, GLsizei width, GLsizei height, GLint magFilter, GLint minFilter)
{
    mtx_lock(&gl::g_mtxGlContext);
    WindowBindGlContext(app::g_pWindow);
    defer(
        WindowUnbindGlContext(app::g_pWindow);
        mtx_unlock(&gl::g_mtxGlContext);
    );

    glGenTextures(1, &s->id);
    glBindTexture(GL_TEXTURE_2D, s->id);
    /* set the texture wrapping parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texMode);
    /* set texture filtering parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

    /* NOTE: simpler way of swapping channels */
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);*/
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);*/

    /* load image, create texture and generate mipmaps */
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pData);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void
ImgSetMonochrome(Img* s, u8* pData, u32 width, u32 height)
{
    mtx_lock(&gl::g_mtxGlContext);
    WindowBindGlContext(app::g_pWindow);
    defer(
        WindowUnbindGlContext(app::g_pWindow);
        mtx_unlock(&gl::g_mtxGlContext);
    );

    s->width = width;
    s->height = height;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &s->id);
    glBindTexture(GL_TEXTURE_2D, s->id);
    defer(
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    );

    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);*/
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pData);
}

CubeMapProjections::CubeMapProjections(const math::M4& proj, const math::V3& pos)
    : tms {
        proj * M4LookAt(pos, pos + math::V3{ 1, 0, 0}, math::V3{0,-1, 0}),
        proj * M4LookAt(pos, pos + math::V3{-1, 0, 0}, math::V3{0,-1, 0}),
        proj * M4LookAt(pos, pos + math::V3{ 0, 1, 0}, math::V3{0, 0, 1}),
        proj * M4LookAt(pos, pos + math::V3{ 0,-1, 0}, math::V3{0, 0,-1}),
        proj * M4LookAt(pos, pos + math::V3{ 0, 0, 1}, math::V3{0,-1, 0}),
        proj * M4LookAt(pos, pos + math::V3{ 0, 0,-1}, math::V3{0,-1, 0})
    } {}

ShadowMap
ShadowMapCreate(const int width, const int height)
{
    GLenum none = GL_NONE;
    ShadowMap res {};
    res.t.width = width;
    res.t.height = height;

    glGenTextures(1, &res.t.tex);
    glBindTexture(GL_TEXTURE_2D, res.t.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	f32 borderColor[] {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint defFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defFramebuffer);
    /* set up fbo */
    glGenFramebuffers(1, &res.t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, res.t.fbo);

    glDrawBuffers(1, &none);
    glReadBuffer(GL_NONE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, res.t.tex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, res.t.tex);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
        LOG_FATAL("glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE\n"); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return res;
}

CubeMap
CubeMapShadowMapCreate(const int width, const int height)
{
    GLuint depthCubeMap;
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);

    for (GLuint i = 0; i < 6; i++)
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_DEPTH_COMPONENT, width, height,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
        );

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    GLenum none = GL_NONE;
    glDrawBuffers(1, &none);
    glReadBuffer(GL_NONE);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
        LOG_FATAL("glCheckFramebufferStatus != GL_FRAMEBUFFER_COMPLETE\n"); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {fbo, depthCubeMap, width, height};
}

CubeMap
skyBoxCreate(String sFaces[6])
{
    Arena al(SIZE_1M * 6);
    defer( ArenaFreeAll(&al) );

    CubeMap cmNew {};

    glGenTextures(1, &cmNew.t.tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cmNew.t.tex);

    for (u32 i = 0; i < 6; i++)
    {
        Data tex = loadBMP(&al.super, sFaces[i], true);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, tex.format, tex.width, tex.height,
            0, tex.format, GL_UNSIGNED_BYTE, VecData(&tex.aData)
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cmNew;
}

Data
loadBMP(IAllocator* pAlloc, String path, bool flip)
{
    u32 imageDataAddress;
    u32 width;
    u32 height;
    u32 nPixels;
    u16 bitDepth;
    u8 byteDepth;

    parser::Bin p(pAlloc);
    parser::BinLoadFile(&p, path);
    auto BM = parser::BinReadString(&p, 2);

    if (BM != "BM")
        LOG_FATAL("BM: '{}', bmp file should have 'BM' as first 2 bytes\n", BM);

    parser::BinSkipBytes(&p, 8);
    imageDataAddress = parser::BinRead32(&p);

#ifdef D_TEXTURE
    LOG_OK("imageDataAddress: {}\n", imageDataAddress);
#endif

    parser::BinSkipBytes(&p, 4);
    width = parser::BinRead32(&p);
    height = parser::BinRead32(&p);
#ifdef D_TEXTURE
    LOG_OK("width: {}, height: {}\n", width, height);
#endif

    [[maybe_unused]] auto colorPlane = parser::BinRead16(&p);
#ifdef D_TEXTURE
    LOG_OK("colorPlane: {}\n", colorPlane);
#endif

    GLint format = GL_RGB;
    bitDepth = parser::BinRead16(&p);
#ifdef D_TEXTURE
    LOG_OK("bitDepth: {}\n", bitDepth);
#endif

    switch (bitDepth)
    {
        case 24:
            format = GL_RGB;
            break;

        case 32:
            format = GL_RGBA;
            break;

        default:
            LOG_WARN("support only for 32 and 24 bit bmp's, read '{}', setting to GL_RGB\n", bitDepth);
            break;
    }

    bitDepth = 32; /* use RGBA anyway */
    nPixels = width * height;
    byteDepth = bitDepth / 8;
#ifdef D_TEXTURE
    LOG_OK("nPixels: {}, byteDepth: {}, format: {}\n", nPixels, byteDepth, format);
#endif
    Vec<u8> pixels(pAlloc, nPixels * byteDepth);

    p.pos = imageDataAddress;

    switch (format)
    {
        default:
        case GL_RGB:
            flipCpyBGRtoRGBA(VecData(&pixels), (u8*)(&p[p.pos]), width, height, flip);
            format = GL_RGBA;
            break;

        case GL_RGBA:
            flipCpyBGRAtoRGBA(VecData(&pixels), (u8*)(&p[p.pos]), width, height, flip);
            break;
    }

    return {
        .aData = pixels,
        .width = width,
        .height = height,
        .bitDepth = bitDepth,
        .format = format
    };
}

Framebuffer
FramebufferCreate(const GLsizei width, const GLsizei height)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* generate texture */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* attach it to currently bound framebuffer object */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_FATAL("framebuffer '{}' creation failed\n", fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {
        .fbo = fbo,
        .tex = tex,
        .width = width,
        .height = height
    };
}

#ifndef NDEBUG
[[maybe_unused]] static void
printPack(String s, __m128i m)
{
    u32 f[4];
    memcpy(f, &m, sizeof(f));
    COUT("'%.*s': %08x, %08x, %08x, %08x\n", s.size, s.pData, f[0], f[1], f[2], f[3]);
};
#endif

[[maybe_unused]] static u32
swapRedBlueBits(u32 col)
{
    u32 r = col & 0x00'ff'00'00;
    u32 b = col & 0x00'00'00'ff;
    return (col & 0xff'00'ff'00) | (r >> (4*4)) | (b << (4*4));
};

void
flipCpyBGRAtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    u32* d = (u32*)(dest);
    u32* s = (u32*)(src);

    for (int y = 0; y < height; y++, f += inc)
    {
        for (int x = 0; x < width; x += 4)
        {
            __m128i pack = _mm_loadu_si128((__m128i*)(&s[y*width + x]));
            __m128i redBits = _mm_and_si128(pack, _mm_set1_epi32(0x00'ff'00'00));
            __m128i blueBits = _mm_and_si128(pack, _mm_set1_epi32(0x00'00'00'ff));
            pack = _mm_and_si128(pack, _mm_set1_epi32(0xff'00'ff'00));

            /* https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#techs=SSE_ALL&ig_expand=3975,627,305,2929,627&cats=Shift */
            redBits = _mm_bsrli_si128(redBits, 2); /* simd bitshifts are in bytes: 'dst[127:0] := a[127:0] << (tmp*8)' */
            blueBits = _mm_bslli_si128(blueBits, 2);

            pack = _mm_or_si128(_mm_or_si128(pack, redBits), blueBits);
            _mm_storeu_si128((__m128i*)(&d[(y-f)*width + x]), pack);
        }
    }
};

void
flipCpyBGRtoRGB(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    constexpr int nComponents = 3;
    width = width * nComponents;

    auto at = [=](int x, int y, int z) -> int {
        return y*width + x + z;
    };

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += nComponents)
        {
            dest[at(x, y-f, 0)] = src[at(x, y, 2)];
            dest[at(x, y-f, 1)] = src[at(x, y, 1)];
            dest[at(x, y-f, 2)] = src[at(x, y, 0)];
        }
        f += inc;
    }
};

void
flipCpyBGRtoRGBA(u8* dest, u8* src, int width, int height, bool vertFlip)
{
    int f = vertFlip ? -(height - 1) : 0;
    int inc = vertFlip ? 2 : 0;

    constexpr int rgbComp = 3;
    constexpr int rgbaComp = 4;

    int rgbWidth = width * rgbComp;
    int rgbaWidth = width * rgbaComp;

    auto at = [](int width, int x, int y, int z) -> int {
        return y*width + x + z;
    };

    for (int y = 0; y < height; y++)
    {
        for (int xSrc = 0, xDest = 0; xSrc < rgbWidth; xSrc += rgbComp, xDest += rgbaComp)
        {
            dest[at(rgbaWidth, xDest, y-f, 0)] = src[at(rgbWidth, xSrc, y, 2)];
            dest[at(rgbaWidth, xDest, y-f, 1)] = src[at(rgbWidth, xSrc, y, 1)];
            dest[at(rgbaWidth, xDest, y-f, 2)] = src[at(rgbWidth, xSrc, y, 0)];
            dest[at(rgbaWidth, xDest, y-f, 3)] = 0xff;
        }
        f += inc;
    }
};

} /* namespace texture */
