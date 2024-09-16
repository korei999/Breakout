#include "TTF.hh"

#include "adt/logs.hh"
#include "parser/Bin.hh"
#include "adt/Vec.hh"

namespace parser
{

constexpr f32
TTF_F2DOT14ToF32(TTF_F2DOT14 x)
{
    f32 ret = f32((0xc000 & x) >> 14);
    ret += (f32(x & 0x3fff)) / f32(0x3fff);

    return ret;
}

static void TTFParse(TTF* s);

void
TTFLoad(TTF* s, String path)
{
    BinLoadFile(&s->p, path);
    TTFParse(s);
}

static void
TTFParse(TTF* s)
{
    if (s->p.sFile.size == 0) LG_FATAL("unable to parse empty file\n");

    u32 sfntVersion = BinRead32Rev(&s->p);
    u16 numTables = BinRead16Rev(&s->p);
    u16 searchRange = BinRead16Rev(&s->p);
    u16 entrySelector = BinRead16Rev(&s->p);
    u16 rangeShift = BinRead16Rev(&s->p);

#ifdef D_TTF
    u16 _searchRangeCheck = pow(2, floor(log2(numTables))) * 16;
    assert(searchRange == _searchRangeCheck);

    u16 _entrySelectorCheck = (u16)floor(log2(numTables));
    assert(entrySelector == _entrySelectorCheck);

    u16 _rangeShiftCheck = numTables*16 - searchRange;
    assert(rangeShift == _rangeShiftCheck);

    LOG_GOOD("sfntVersion: %#0x\n", sfntVersion);
    LOG_GOOD("numTables: %d\n", numTables);
    LOG_GOOD("searchRange: %d\n", searchRange);
    LOG_GOOD("entrySelector: %d\n", entrySelector);
    LOG_GOOD("rangeShift: %d\n", rangeShift);
#endif

    Vec<TableRecord> tableRecords(s->p.pAlloc, numTables); 
}

void
TTFDestroy(TTF* s)
{
    // TODO:
}

} /* namespace parser */
