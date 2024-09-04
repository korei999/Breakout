#pragma once

#include "adt/Allocator.hh"
#include "adt/String.hh"

using namespace adt;

namespace parser
{

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

bool BinLoadFile(Bin* s, String path);
void BinSkipBytes(Bin* s, u32 n);
String BinReadString(Bin* s, u32 bytes);
u8 BinRead8(Bin* s);
u16 BinRead16(Bin* s);
u32 BinRead32(Bin* s);
u64 BinRead64(Bin* s);
void BinSetPos(Bin* s, u32 p);
bool BinFinished(Bin* s);

template <typename T>
ADT_NO_UB
T
readTypeBytes(String vec, u32 at)
{
    return *(T*)(&vec[at]);
}

} /* namespace parser */
