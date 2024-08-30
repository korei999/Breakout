#pragma once

#include "String.hh"
#include "Array.hh"

#include <stdio.h>

namespace adt
{
namespace file
{

inline String load(Allocator* pAlloc, String path);
inline Array<u8> loadToCharArray(Allocator* pAlloc, String path);
inline String replacePathEnding(Allocator* pAlloc, String path, String sEnding);

inline String
load(Allocator* pAlloc, String path)
{
    String ret;

    auto sn = StringAlloc(pAlloc, path);

    FILE* pf = fopen(sn.pData, "rb");
    if (pf)
    {
        fseek(pf, 0, SEEK_END);
        long size = ftell(pf) + 1;
        rewind(pf);

        ret.pData = (char*)alloc(pAlloc, size, sizeof(char));
        ret.size = size - 1;
        fread(ret.pData, 1, ret.size, pf);

        fclose(pf);
    }

    return ret;
}

inline Array<u8>
loadToCharArray(Allocator* pAlloc, String path)
{
    Array<u8> ret(pAlloc);

    FILE* pf = fopen(path.pData, "rb");
    if (pf)
    {
        fseek(pf, 0, SEEK_END);
        long size = ftell(pf);
        rewind(pf);

        ArraySetSize(&ret, size + 1);
        ret.size = size;
        fread(ret.pData, 1, size, pf);

        fclose(pf);
    }

    return ret;
}

inline String
replacePathEnding(Allocator* pAlloc, String path, String sEnding)
{
    auto lastSlash = StringLastOf(path, '/');
    String sNoEnding = {&path[0], lastSlash + 1};
    auto r = StringCat(pAlloc, sNoEnding, sEnding);
    return r;
}

} /* namespace file */
} /* namespace adt */
