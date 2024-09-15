#pragma once

#include "String.hh"
#include "logs.hh"
#include "Option.hh"

namespace adt
{
namespace file
{

inline Option<String> load(Allocator* pAlloc, String path);
inline String replacePathEnding(Allocator* pAlloc, String path, String sEnding);

[[nodiscard]]
inline Option<String>
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
    else LOG_WARN("ret(%p): Error opening '%.*s' file\n", pf, path.size, path.pData);

    free(pAlloc, sn.pData);
    return ret;
}

[[nodiscard]]
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
