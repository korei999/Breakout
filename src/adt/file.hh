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
load(Allocator* pAlloc, String path)
{
    Option<String> ret {};

    FILE* pf = fopen(path.pData, "rb");
    if (!pf)
    {
        LOG_WARN("Error opening '{}' file\n", path);
        return ret;
    }
    defer(fclose(pf));

    fseek(pf, 0, SEEK_END);
    long size = ftell(pf) + 1;
    rewind(pf);

    ret.data.pData = (char*)alloc(pAlloc, size, sizeof(char));
    ret.data.size = size - 1;
    fread(ret.data.pData, 1, ret.data.size, pf);
    ret.bHasValue = true;

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
