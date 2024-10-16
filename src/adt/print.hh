#pragma once

#include "String.hh"
#include "utils.hh"

#include <cmath>
#include <cstdlib>
#include <type_traits>

namespace adt
{
namespace print
{

enum BASE : u8 { TWO = 2, EIGHT = 8, TEN = 10, SIXTEEN = 16 };

struct FormatArgs
{
    u16 maxLen = NPOS16;
    u16 maxFloatLen = NPOS16;
    BASE eBase = BASE::TEN;
    bool bHexHash = false;
    bool bAlwaysShowSign = false;
};

struct Context
{
    String fmt {};
    char* const pBuff {};
    const u32 buffSize {};
    u32 buffIdx {};
    u32 fmtIdx {};
};

template<typename... ARGS_T> constexpr void cout(const String fmt, const ARGS_T&... tArgs);
template<typename... ARGS_T> constexpr void cerr(const String fmt, const ARGS_T&... tArgs);

constexpr u32
printArgs(Context ctx)
{
    u32 nRead = 0;
    for (u32 i = ctx.fmtIdx; i < ctx.fmt.size; ++i, ++nRead)
    {
        if (ctx.buffIdx >= ctx.buffSize) break;
        ctx.pBuff[ctx.buffIdx++] = ctx.fmt[i];
    }

    return nRead;
}

inline u32
parseFormatArg(const String fmt, u32 fmtIdx, FormatArgs* pArgs)
{
    u32 nRead = 1;
    bool bDone = false;
    bool bColon = false;
    bool bFloatPresicion = false;
    bool bHash = false;
    bool bHex = false;
    bool bBinary = false;
    bool bAlwaysShowSign = false;

    char buff[32] {};

    for (u32 i = fmtIdx + 1; i < fmt.size; ++i, ++nRead)
    {
        if (bDone) break;

        if (bHash && bHex)
        {
            bHash = bHex = false;
            pArgs->eBase = BASE::SIXTEEN;
            pArgs->bHexHash = true;
        }
        else if (bHex)
        {
            bHex = false;
            pArgs->eBase = BASE::SIXTEEN;
        }
        else if (bBinary)
        {
            bBinary = false;
            pArgs->eBase = BASE::TWO;
        }
        else if (bAlwaysShowSign)
        {
            bAlwaysShowSign = false;
            pArgs->bAlwaysShowSign = true;
        }
        else if (bFloatPresicion)
        {
            bFloatPresicion = false;
            u32 bIdx = 0;
            while (bIdx < sizeof(buff) - 1 && i < fmt.size && fmt[i] != '}')
            {
                buff[bIdx++] = fmt[i++];
                ++nRead;
            }

            pArgs->maxFloatLen = atoi(buff);
        }

        if (bColon)
        {
            if (fmt[i] == '.')
            {
                bFloatPresicion = true;
                continue;
            }
            else if (fmt[i] == '#')
            {
                bHash = true;
                continue;
            }
            else if (fmt[i] == 'x')
            {
                bHex = true;
                continue;
            }
            else if (fmt[i] == 'b')
            {
                bBinary = true;
                continue;
            }
            else if (fmt[i] == '+')
            {
                bAlwaysShowSign = true;
                continue;
            }
        }

        if (fmt[i] == '}')
            bDone = true;
        else if (fmt[i] == ':')
            bColon = true;
    }

    return nRead;
}

/* C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3. */
template<typename INT_T> requires std::is_integral_v<INT_T>
constexpr char*
intToBuffer(INT_T x, char* pDest, u32 dstSize, FormatArgs fmtArgs)
{
    if (fmtArgs.eBase < 2 || fmtArgs.eBase > 36)
    {
        *pDest = '\0';
        return pDest;
    }

    char* p0 = pDest, * p1 = pDest;
    char tmpChar {};
    INT_T tmpVal {};

    constexpr String map = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

    u32 i = 0;

    auto pushChar = [&](char c) -> bool {
        if (i < dstSize)
        {
            *p0++ = c;
            ++i;
            return true;
        }

        return false;
    };

    do {
        tmpVal = x;
        x /= fmtArgs.eBase;
        if (!pushChar(map[35 + (tmpVal - x*fmtArgs.eBase)])) break;
    } while (x);

    if (tmpVal < 0 && !fmtArgs.bAlwaysShowSign)
        if (!fmtArgs.bHexHash)
            pushChar('-');

    if (fmtArgs.bAlwaysShowSign)
    {
        if (tmpVal >= 0)
            pushChar('+');
        else pushChar('-');
    }

    if (fmtArgs.eBase == BASE::SIXTEEN && fmtArgs.bHexHash)
    {
        pushChar('x');
        pushChar('0');
    }

    *p0-- = '\0';

    /* Reverse the string */
    while (p1 < p0)
    {
        tmpChar = *p0;
        *p0-- = *p1;
        *p1++ = tmpChar;
    }

    return pDest;
}

constexpr u32
copyBackToBuffer(Context ctx, char* pSrc, u32 srcSize)
{
    u32 nRead = 0;

    for (u32 i = 0; pSrc[i] != '\0' && i < srcSize && ctx.buffIdx < ctx.buffSize; ++i, ++nRead)
        ctx.pBuff[ctx.buffIdx++] = pSrc[i];

    return nRead;
}

constexpr u32
formatToContext(Context ctx, [[maybe_unused]]  FormatArgs fmtArgs, const String& str)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;

    u32 nRead = 0;
    for (u32 i = 0; i < str.size && buffIdx < buffSize; ++i, ++nRead)
        pBuff[buffIdx++] = str[i];

    return nRead;
}

constexpr u32
formatToContext(Context ctx, FormatArgs fmtArgs, const char* str)
{
    return formatToContext(ctx, fmtArgs, String(str));
}

constexpr u32
formatToContext(Context ctx, FormatArgs fmtArgs, char* const& pNullTerm)
{
    return formatToContext(ctx, fmtArgs, String(pNullTerm));
}

template<typename INT_T> requires std::is_integral_v<INT_T>
constexpr u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const INT_T& x)
{
    char buff[32] {};
    char* p = intToBuffer(x, buff, sizeof(buff), fmtArgs);

    return copyBackToBuffer(ctx, p, sizeof(buff));
}

inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, const f32 x)
{
    char nbuff[32] {};
    snprintf(nbuff, sizeof(nbuff), "%.*f", fmtArgs.maxFloatLen, x);

    return copyBackToBuffer(ctx, nbuff, sizeof(nbuff));
}

inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, const f64 x)
{
    char nbuff[32] {};
    snprintf(nbuff, sizeof(nbuff), "%.*lf", fmtArgs.maxFloatLen, x);

    return copyBackToBuffer(ctx, nbuff, sizeof(nbuff));
}

inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const wchar_t x)
{
    char nbuff[4] {};
    snprintf(nbuff, sizeof(nbuff), "%lc", x);

    return copyBackToBuffer(ctx, nbuff, sizeof(nbuff));
}

inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const char x)
{
    char nbuff[4] {};
    snprintf(nbuff, sizeof(nbuff), "%c", x);

    return copyBackToBuffer(ctx, nbuff, sizeof(nbuff));
}

inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, [[maybe_unused]] null nullPtr)
{
    return formatToContext(ctx, fmtArgs, String("nullptr"));
}

template<typename PTR_T> requires std::is_pointer_v<PTR_T>
inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, PTR_T p)
{
    fmtArgs.bHexHash = true;
    fmtArgs.eBase = BASE::SIXTEEN;
    return formatToContext(ctx, fmtArgs, u64(p));
}

template<typename T, typename... ARGS_T>
constexpr u32
printArgs(Context ctx, const T& tFirst, const ARGS_T&... tArgs)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;
    auto& fmt = ctx.fmt;
    auto& fmtIdx = ctx.fmtIdx;

    if (fmtIdx >= fmt.size) return 0;

    u32 nRead = 0;
    bool bArg = false;

    u32 i = fmtIdx;
    for (; i < fmt.size; ++i, ++nRead)
    {
        if (buffIdx >= buffSize) return nRead;

        FormatArgs fmtArgs {};

        if (fmt[i] == '{')
        {
            if (i + 1 < fmt.size && fmt[i + 1] == '{')
            {
                i += 1, nRead += 1;
                bArg = false;
            }
            else bArg = true;
        }

        if (bArg)
        {
            u32 add = parseFormatArg(fmt, i, &fmtArgs);
            u32 addBuff = formatToContext(ctx, fmtArgs, tFirst);
            buffIdx += addBuff;
            i += add;
            nRead += addBuff;
            break;
        }
        else pBuff[buffIdx++] = fmt[i];
    }

    fmtIdx = i;
    nRead += printArgs(ctx, tArgs...);

    return nRead;
}

template<typename... ARGS_T>
constexpr void
toFILE(FILE* fp, const String fmt, const ARGS_T&... tArgs)
{
    /* TODO: set size / allow allocation maybe */
    char aBuff[1024] {};
    Context ctx {fmt, aBuff, utils::size(aBuff) - 1};
    printArgs(ctx, tArgs...);
    fputs(aBuff, fp);
}

template<typename... ARGS_T>
constexpr void
toBuffer(char* pBuff, u32 buffSize, const String fmt, const ARGS_T&... tArgs)
{
    Context ctx {fmt, pBuff, buffSize};
    printArgs(ctx, tArgs...);
}

template<typename... ARGS_T>
constexpr void
toString(String* pDest, const String fmt, const ARGS_T&... tArgs)
{
    toBuffer(pDest->pData, pDest->size, fmt, tArgs...);
}

template<typename... ARGS_T>
constexpr void
cout(const String fmt, const ARGS_T&... tArgs)
{
    toFILE(stdout, fmt, tArgs...);
}

template<typename... ARGS_T>
constexpr void
cerr(const String fmt, const ARGS_T&... tArgs)
{
    toFILE(stderr, fmt, tArgs...);
}

} /* namespace print */
} /* namespace adt */
