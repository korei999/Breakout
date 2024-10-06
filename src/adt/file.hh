#pragma once

#include "String.hh"
#include "logs.hh"
#include "Option.hh"
#include "defer.hh"

namespace adt
{
namespace file
{

[[nodiscard]]
inline Option<String>
load(Allocator* pAlloc, String sPath)
{
    FILE* pf = fopen(sPath.pData, "rb");
    if (!pf)
    {
        LOG_WARN("Error opening '{}' file\n", sPath);
        return {};
    }
    defer(fclose(pf));

    String ret {};

    fseek(pf, 0, SEEK_END);
    long size = ftell(pf) + 1;
    rewind(pf);

    ret.pData = (char*)alloc(pAlloc, size, sizeof(char));
    ret.size = size - 1;
    fread(ret.pData, 1, ret.size, pf);

    return {ret, true};
}

[[nodiscard]]
inline String
replacePathEnding(Allocator* pAlloc, String sPath, String sEnding)
{
    auto lastSlash = StringLastOf(sPath, '/');
    String sNoEnding = {&sPath[0], lastSlash + 1};
    auto r = StringCat(pAlloc, sNoEnding, sEnding);
    return r;
}

} /* namespace file */
} /* namespace adt */
