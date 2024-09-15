#include "Bin.hh"
#include "adt/file.hh"

namespace parser
{

bool 
BinLoadFile(Bin* s, String path)
{
    s->sPath = StringAlloc(s->pAlloc, path);
    s->pos = 0;

    Option<String> rs = file::load(s->pAlloc, path);
    if (!rs) LOG_FATAL("error opening file: '%.*s'\n", path.size, path.pData);

    s->sFile = rs.data;

    return s->sFile.pData != nullptr;
}

void 
BinSkipBytes(Bin* s, u32 n)
{
    s->pos += n;
}

String
BinReadString(Bin* s, u32 bytes)
{
    String ret(&s->sFile[s->pos], bytes);
    s->pos += bytes;
    return ret;
}

u8
BinRead8(Bin* s)
{
    auto ret = readTypeBytes<u8>(s->sFile, s->pos);
    s->pos += 1;
    return ret;
}

u16
BinRead16(Bin* s)
{
    auto ret = readTypeBytes<u16>(s->sFile, s->pos);
    s->pos += 2;
    return ret;
}

u32
BinRead32(Bin* s)
{
    auto ret = readTypeBytes<u32>(s->sFile, s->pos);
    s->pos += 4;
    return ret;
}

u64
BinRead64(Bin* s)
{
    auto ret = readTypeBytes<u64>(s->sFile, s->pos);
    s->pos += 8;
    return ret;
}

void
BinSetPos(Bin* s, u32 p)
{
    s->pos = p;
}

bool
BinFinished(Bin* s)
{
    return s->pos >= s->sFile.size;
}

} /* namespace parser */
