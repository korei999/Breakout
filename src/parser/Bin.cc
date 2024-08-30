#include "Bin.hh"
#include "adt/file.hh"

namespace parser
{

Bin::Bin(Allocator* p, String path)
    : Bin(p) 
{
    BinLoadFile(this, path); 
}

void 
BinLoadFile(Bin* s, String path)
{
    s->start = s->end = 0;
    s->file = file::load(s->pAlloc, path);
}

void 
BinSkipBytes(Bin* s, u32 n)
{
    s->start = s->end = s->end + n;
}

String
BinReadString(Bin* s, u32 size)
{
    String ret(&s->file[s->start], size - s->start);
    s->start = s->end = s->end + size;
    return ret;
}

u8
BinRead8(Bin* s)
{
    auto ret = readTypeBytes<u8>(s->file, s->start);
    s->start = s->end = s->end + 1;
    return ret;
}

u16
BinRead16(Bin* s)
{
    auto ret = readTypeBytes<u16>(s->file, s->start);
    s->start = s->end = s->end + 2;
    return ret;
}

u32
BinRead32(Bin* s)
{
    auto ret = readTypeBytes<u32>(s->file, s->start);
    s->start = s->end = s->end + 4;
    return ret;
}

u64
BinRead64(Bin* s)
{
    auto ret = readTypeBytes<u64>(s->file, s->start);
    s->start = s->end = s->end + 8;
    return ret;
}

void
BinSetPos(Bin* s, u32 p)
{
    s->start = s->end = p;
}

bool
BinFinished(Bin* s)
{
    return s->start >= s->file.size;
}

} /* namespace parser */
