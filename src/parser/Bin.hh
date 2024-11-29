#pragma once

#include "adt/Allocator.hh"
#include "adt/String.hh"
#include "adt/file.hh"

#include <bit>

using namespace adt;

namespace parser
{

template <typename T>
ADT_NO_UB inline T
readTypeBytes(String s, u32 at)
{
    return *(T*)(&s[at]);
}

struct Bin
{
    Allocator* pAlloc;
    String sFile;
    String sPath;
    u32 pos;

    Bin() = default;
    Bin(Allocator* p) : pAlloc(p) {}

    char& operator[](u32 i) { return sFile[i]; };
};

inline bool 
BinLoadFile(Bin* s, String path)
{
    s->sPath = StringAlloc(s->pAlloc, path);
    s->pos = 0;

    Opt<String> rs = file::load(s->pAlloc, path);
    if (!rs) LOG_FATAL("error opening file: '{}'\n", path);

    s->sFile = rs.data;

    return s->sFile.pData != nullptr;
}

inline void 
BinSkipBytes(Bin* s, u32 n)
{
    s->pos += n;
}

inline String
BinReadString(Bin* s, u32 bytes)
{
    String ret(&s->sFile[s->pos], bytes);
    s->pos += bytes;
    return ret;
}

inline u8
BinRead8(Bin* s)
{
    auto ret = readTypeBytes<u8>(s->sFile, s->pos);
    s->pos += 1;
    return ret;
}

inline u16
BinRead16(Bin* s)
{
    auto ret = readTypeBytes<u16>(s->sFile, s->pos);
    s->pos += 2;
    return ret;
}

inline u16
BinRead16Rev(Bin* s)
{
    return std::byteswap(BinRead16(s));
}

inline u32
BinRead32(Bin* s)
{
    auto ret = readTypeBytes<u32>(s->sFile, s->pos);
    s->pos += 4;
    return ret;
}

inline u32
BinRead32Rev(Bin* s)
{
    return std::byteswap(BinRead32(s));
}

inline u64
BinRead64(Bin* s)
{
    auto ret = readTypeBytes<u64>(s->sFile, s->pos);
    s->pos += 8;
    return ret;
}

inline u64
BinRead64Rev(Bin* s)
{
    return std::byteswap(BinRead64(s));
}

inline bool
BinFinished(Bin* s)
{
    return s->pos >= s->sFile.size;
}

} /* namespace parser */
