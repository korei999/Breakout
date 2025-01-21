#pragma once

#include "adt/TwoDSpan.hh"

#include <cstdio>

using namespace adt;

namespace tga
{

enum class PIXEL_TYPE : u8 { RGB, RGBA };

struct PixelRGB
{
    u8 r {};
    u8 g {};
    u8 b {};
};

struct PixelRGBA : PixelRGB
{
    u8 a {};
};

#pragma pack(1)
struct Header
{
    s8 idLen {};
    s8 colorMapType {};
    s8 dataTypeCode {}; /* 2 (RGB) */
    s16 colorMapOrigin {};
    s16 colorMapLenght {};
    s8 colorMapDepth {};
    s16 xOrigin {};
    s16 yOrigin {};
    s16 width {};
    s16 height {};
    s8 bitsPerPixel {};
    s8 imageDescriptor {};
};
#pragma pack()

struct Image
{
    TwoDSpan<PixelRGBA> m_spBuff {};

    /* */

    Image() = default;

    /* */

    PixelRGBA& operator()(ssize x, ssize y) { return m_spBuff(x, y); }
    const PixelRGBA& operator()(ssize x, ssize y) const { return m_spBuff(x, y); }
    auto data() { return m_spBuff.data(); }

    bool read(TwoDSpan<PixelRGBA> spData);
    void writeToFile(FILE* fp);
};

inline bool
Image::read(TwoDSpan<PixelRGBA> spData)
{
    m_spBuff = {spData.data(), spData.getWidth(), spData.getHeight()};

    return true;
}

inline void
Image::writeToFile(FILE* fp)
{
    Header header {
        .idLen = 0,
        .colorMapType = 0,
        .dataTypeCode = 2,
        .colorMapOrigin = 0,
        .colorMapLenght = 0,
        .colorMapDepth = 0,
        .xOrigin = 0,
        .yOrigin = 0,
        .width = m_spBuff.getWidth(),
        .height = m_spBuff.getHeight(),
        .bitsPerPixel = 24,
        .imageDescriptor = 0,
    };

    fwrite(&header, sizeof(Header), 1, fp);

    fwrite(
        m_spBuff.data(),
        m_spBuff.getWidth() * m_spBuff.getHeight() * sizeof(*m_spBuff.data()),
        1,
        fp
    );
}

};
