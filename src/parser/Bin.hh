#pragma once

#include "adt/Allocator.hh"
#include "adt/String.hh"
#include "adt/file.hh"

using namespace adt;

namespace parser
{

template <typename T>
ADT_NO_UB
constexpr T
readTypeBytes(String s, u32 at)
{
    return *(T*)(&s[at]);
}

constexpr u16
swapBytes(u16 x)
{
#if defined __clang__ || __GNUC__
    return __builtin_bswap16(x);
#else
    return ((x & 0x00ff) << 4*2) | ((x & 0xff00) >> 4*2);
#endif
}

constexpr u32
swapBytes(u32 x)
{
#if defined __clang__ || __GNUC__
    return __builtin_bswap32(x);
#else
    return ((x & 0x000000ff) << 4*6) |
           ((x & 0x0000ff00) << 4*2) |
           ((x & 0x00ff0000) >> 4*2) |
           ((x & 0xff000000) >> 4*6);
#endif
}

constexpr u64
swapBytes(u64 x)
{
#if defined __clang__ || __GNUC__
    return __builtin_bswap64(x);
#else
    return ((x & 0x0000'0000'00ffLU) << 4*10) |
           ((x & 0x0000'0000'ff00LU) << 4*6)  |
           ((x & 0x0000'00ff'0000LU) << 4*2)  |
           ((x & 0x0000'ff00'0000LU) >> 4*2)  |
           ((x & 0x00ff'0000'0000LU) >> 4*6)  |
           ((x & 0xff00'0000'0000LU) >> 4*10);
#endif
}

struct Bin
{
    Allocator* pAlloc;
    String sFile;
    String sPath;
    u32 pos;

    constexpr Bin() = default;
    constexpr Bin(Allocator* p) : pAlloc(p) {}

    char& operator[](u32 i) { return sFile[i]; };
};

inline bool 
BinLoadFile(Bin* s, String path)
{
    s->sPath = StringAlloc(s->pAlloc, path);
    s->pos = 0;

    Option<String> rs = file::load(s->pAlloc, path);
    if (!rs) LOG_FATAL("error opening file: '%.*s'\n", path.size, path.pData);

    s->sFile = rs.data;

    return s->sFile.pData != nullptr;
}

constexpr void 
BinSkipBytes(Bin* s, u32 n)
{
    s->pos += n;
}

constexpr String
BinReadString(Bin* s, u32 bytes)
{
    String ret(&s->sFile[s->pos], bytes);
    s->pos += bytes;
    return ret;
}

constexpr u8
BinRead8(Bin* s)
{
    auto ret = readTypeBytes<u8>(s->sFile, s->pos);
    s->pos += 1;
    return ret;
}

constexpr u16
BinRead16(Bin* s)
{
    auto ret = readTypeBytes<u16>(s->sFile, s->pos);
    s->pos += 2;
    return ret;
}

constexpr u16
BinRead16Rev(Bin* s)
{
    return swapBytes(BinRead16(s));
}

constexpr u32
BinRead32(Bin* s)
{
    auto ret = readTypeBytes<u32>(s->sFile, s->pos);
    s->pos += 4;
    return ret;
}

constexpr u32
BinRead32Rev(Bin* s)
{
    return swapBytes(BinRead32(s));
}

constexpr u64
BinRead64(Bin* s)
{
    auto ret = readTypeBytes<u64>(s->sFile, s->pos);
    s->pos += 8;
    return ret;
}

constexpr u64
BinRead64Rev(Bin* s)
{
    return swapBytes(BinRead64(s));
}

constexpr void
BinSetPos(Bin* s, u32 p)
{
    s->pos = p;
}

constexpr bool
BinFinished(Bin* s)
{
    return s->pos >= s->sFile.size;
}

} /* namespace parser */
