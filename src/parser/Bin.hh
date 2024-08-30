#pragma once

#include "adt/Allocator.hh"
#include "adt/String.hh"

using namespace adt;

namespace parser
{

struct Bin
{
    Allocator* pAlloc;
    String word;
    String file;
    u32 start;
    u32 end;

    Bin() = default;
    Bin(Allocator* p) : pAlloc(p) {}
    Bin(Allocator* p, String path);

    char& operator[](u32 i) { return file[i]; };
};

void BinLoadFile(Bin* s, String path);
void BinSkipBytes(Bin* s, u32 n);
String BinReadString(Bin* s, u32 size);
u8 BinRead8(Bin* s);
u16 BinRead16(Bin* s);
u32 BinRead32(Bin* s);
u64 BinRead64(Bin* s);
void BinSetPos(Bin* s, u32 p);
bool BinFinished(Bin* s);

template <typename T>
#if defined __clang__ ||  __GNUC__
__attribute__((no_sanitize("undefined"))) /* it's fine */
#endif
T
readTypeBytes(String vec, u32 at)
{
    return *(T*)(&vec[at]);
}

} /* namespace parser */
