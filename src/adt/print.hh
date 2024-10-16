#pragma once

#include "String.hh"
#include "utils.hh"

#include <cmath>
#include <concepts>
#include <cstdlib>

namespace adt
{
namespace print
{

struct FormatArgs
{
    u16 maxLen = NPOS16;
    u16 maxFloatLen = NPOS16;
};

struct Context
{
    String fmt {};
    char* const pBuff {};
    const u32 buffSize {};
    u32 buffIdx {};
    u32 fmtIdx {};

    Context() = default;
    Context(const String& _fmt, char* _pBuff, const u32 _buffSize) : fmt(_fmt), pBuff(_pBuff), buffSize(_buffSize) {}
};

template<typename... ARGS_T> constexpr void cout(const String fmt, const ARGS_T&... tArgs);
template<typename... ARGS_T> constexpr void cerr(const String fmt, const ARGS_T&... tArgs);

constexpr u32
printArgs(Context ctx)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;
    auto& fmt = ctx.fmt;
    auto& fmtIdx = ctx.fmtIdx;

    u32 nRead = 0;
    for (u32 i = fmtIdx; i < fmt.size; ++i, ++nRead)
    {
        if (buffIdx >= buffSize) break;
        pBuff[buffIdx++] = fmt[i];
    }

    return nRead;
}

constexpr u32
parseFormatArg(const String fmt, u32 fmtIdx, FormatArgs* pArgs)
{
    u32 nRead = 1;
    bool bDone = false;
    bool bColon = false;
    bool bFloatPresicion = false;

    char buff[32] {};

    for (u32 i = fmtIdx + 1; i < fmt.size && !bDone; ++i, ++nRead)
    {
        if (bFloatPresicion)
        {
            bFloatPresicion = false;
            u32 bIdx = 0;
            while (bIdx < sizeof(buff) - 1 && i < fmt.size && fmt[i] != '}')
            {
                buff[bIdx++] = fmt[i++];
                ++nRead;
            }

            pArgs->maxFloatLen = std::atoi(buff);
        }

        if (bColon)
        {
            bColon = false;
            if (fmt[i] == '.')
            {
                bFloatPresicion = true;
                continue;
            }
        }

        if (fmt[i] == '}')
        {
            bDone = true;
        }
        else if (fmt[i] == ':')
        {
            bColon = true;
        }
    }

    return nRead;
}

constexpr u32
formatToContext(Context ctx, [[maybe_unused]]  FormatArgs fmtArgs, const String& str)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;

    u32 nRead = 0;

    for (auto c : str)
    {
        if (buffIdx < buffSize)
        {
            pBuff[buffIdx + nRead] = c;
            ++nRead;
        }
    }

    return nRead;
}

template<u32 SIZE>
constexpr u32
formatToContext(Context ctx, FormatArgs fmtArgs, const char (&str)[SIZE])
{
    return formatToContext(ctx, fmtArgs, String(str));
}

constexpr u32
formatToContext(Context ctx, FormatArgs fmtArgs, char* const& pNullTerm)
{
    return formatToContext(ctx, fmtArgs, String(pNullTerm));
}

/* C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3. */
constexpr char*
intToChar(s64 x, char* pRes, int base)
{
    if (base < 2 || base > 36)
    {
        *pRes = '\0';
        return pRes;
    }

    char *ptr = pRes, *ptr1 = pRes, tmpChar;
    s64 tmpVal;

    do {
        tmpVal = x;
        x /= base;
        *ptr++ =
            "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmpVal - x*base)];
    } while (x);

    /* Apply negative sign */
    if (tmpVal < 0)
        *ptr++ = '-';
    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr)
    {
        tmpChar = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmpChar;
    }
    return pRes;
}

constexpr char*
uintToChar(u64 x, char* pRes, int base)
{
    if (base < 2 || base > 36)
    {
        *pRes = '\0';
        return pRes;
    }

    char *ptr = pRes, *ptr1 = pRes, tmpChar;
    u64 tmpVal;

    do {
        tmpVal = x;
        x /= base;
        *ptr++ =
            "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmpVal - x*base)];
    } while (x);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr)
    {
        tmpChar = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmpChar;
    }
    return pRes;
}

constexpr u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const std::integral auto& x)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;

    u32 nRead = 0;

    char buff[50] {};
    auto* what = uintToChar(x, buff, 10);

    u32 i = 0;
    for (; what[i] != '\0' && buffIdx < buffSize; ++i, ++nRead)
        pBuff[buffIdx++] = what[i];

    return nRead;
}

inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, const f32 x)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;

    char nbuff[32] {};
    snprintf(nbuff, sizeof(nbuff) - 1, "%.*f", fmtArgs.maxFloatLen, x);
    char* what = nbuff;

    u32 nRead = 0;
    for (u32 i = 0; what[i] != '\0' && buffIdx < buffSize; ++i, ++nRead)
        pBuff[buffIdx++] = what[i];

    return nRead;
}

inline u32
formatToContext(Context ctx, FormatArgs fmtArgs, const f64 x)
{
    auto& pBuff = ctx.pBuff;
    auto& buffSize = ctx.buffSize;
    auto& buffIdx = ctx.buffIdx;

    char nbuff[32] {};
    snprintf(nbuff, sizeof(nbuff) - 1, "%.*lf", fmtArgs.maxFloatLen, x);
    char* what = nbuff;

    u32 nRead = 0;
    for (u32 i = 0; what[i] != '\0' && buffIdx < buffSize; ++i, ++nRead)
        pBuff[buffIdx++] = what[i];

    return nRead;
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

        FormatArgs args {};

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
            u32 add = parseFormatArg(fmt, i, &args);
            u32 addBuff = formatToContext(ctx, args, tFirst);
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
    char aBuff[512] {};
    Context ctx(fmt, aBuff, utils::size(aBuff) - 1);
    printArgs(ctx, tArgs...);
    fputs(aBuff, fp);
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
