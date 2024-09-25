#include "ttf.hh"

#include <math.h>

namespace parser
{
namespace ttf
{

static void FontParse(Font* s);

constexpr f32
F2Dot14Tof32(F2Dot14 x)
{
    f32 ret = f32((0xc000 & x) >> 14);
    ret += (f32(x & 0x3fff)) / f32(0x3fff);

    return ret;
}

bool
FontLoad(Font* s, String path)
{
    auto bSuc = BinLoadFile(&s->p, path);
    if (!bSuc) LOG_FATAL("BinLoadFile failed: '%.*s'\n", path.size, path.pData);

    FontParse(s);

    return true;
}

constexpr u32
getTableChecksum(u32* pTable, u32 nBytes)
{
    u32 sum = 0;
    u32 nLongs = (nBytes + 3) / 4;
    while (nLongs > 0)
    {
        sum += swapBytes(*pTable);
        nLongs--, pTable++;
    }

    return sum;
}

constexpr String
platformIDToString(u16 platformID)
{
    constexpr String map[] {
        "Unicode", "Mac", "(reserved; do not use)", "Microsoft"
    };

    assert(platformID < utils::size(map));
    return map[platformID];
}

constexpr String
platformSpecificIDToString(u16 platformSpecificID)
{
    constexpr String map[] {
        "Version 1.0",
        "Version 1.1",
        "ISO 10646 1993 (deprecated)",
        "Unicode 2.0 or later (BMP only)",
        "Unicode 2.0 or later (non-BMP characters allowed)",
        "Unicode Variation Sequences",
        "Last Resort"
    };

    assert(platformSpecificID < utils::size(map));
    return map[platformSpecificID];
}

constexpr String
languageIDToString(u16 languageID)
{
    switch (languageID)
    {
        default: return "Uknown";
        case 0: return "English";
        case 1033: return "English United States";
        case 1028: return "Traditional Chinese";
        case 1036: return "French";
        case 1040: return "Italian";
        case 2052: return "Simplified Chinese";
        case 1106: return "Welsh";
        case 1142: return "Latin";
    }
}

static void
readHeadTable(Font* s)
{
    u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    auto fHead = HashMapSearch(&s->tableDirectory.mTableRecords, {"head"});
    assert(fHead);

    s->p.pos = fHead.pData->offset;

    auto& h = s->head;

    h.version.l = BinRead16Rev(&s->p);
    h.version.r = BinRead16Rev(&s->p);
    h.fontRevision.l = BinRead16Rev(&s->p);
    h.fontRevision.r = BinRead16Rev(&s->p);
    h.checkSumAdjustment = BinRead32Rev(&s->p);
    h.magicNumber = BinRead32Rev(&s->p);
    h.flags = BinRead16Rev(&s->p);
    h.unitsPerEm = BinRead16Rev(&s->p);
    h.created = BinRead64Rev(&s->p);
    h.modified = BinRead64Rev(&s->p);
    h.xMin = BinRead16Rev(&s->p);
    h.yMin = BinRead16Rev(&s->p);
    h.xMax = BinRead16Rev(&s->p);
    h.yMax = BinRead16Rev(&s->p);
    h.macStyle = BinRead16Rev(&s->p);
    h.lowestRecPPEM = BinRead16Rev(&s->p);
    h.fontDirectionHint = BinRead16Rev(&s->p);
    h.indexToLocFormat = BinRead16Rev(&s->p);
    h.glyphDataFormat = BinRead16Rev(&s->p);

    LOG(
        "\thead:\n"
        "\t\tversion: %d, %d\n"
        "\t\tfontRevision: %d, %d\n"
        "\t\tcheckSumAdjustment: %u\n"
        "\t\tmagicNumber: %u\n"
        "\t\tflags: %#x\n"
        "\t\tunitsPerEm: %u\n"
        "\t\tcreated: %ld\n"
        "\t\tmodified: %ld\n"
        "\t\txMin: %d\n"
        "\t\tyMin: %d\n"
        "\t\txMax: %d\n"
        "\t\tyMax: %d\n"
        "\t\tmacStyle: %u\n"
        "\t\tlowestRecPPEM: %u\n"
        "\t\tfontDirectionHint: %d\n"
        "\t\tindexToLocFormat: %d\n"
        "\t\tglyphDataFormat: %d\n",
        h.version.l, h.version.r,
        h.fontRevision.l, h.fontRevision.r,
        h.checkSumAdjustment,
        h.magicNumber,
        h.flags,
        h.unitsPerEm,
        h.created,
        h.modified,
        h.xMin,
        h.yMin,
        h.xMax,
        h.yMax,
        h.macStyle,
        h.lowestRecPPEM,
        h.fontDirectionHint,
        h.indexToLocFormat,
        h.glyphDataFormat
    );
}

static u32
getGlyphOffset(Font* s, u32 idx)
{
    auto fLoca = HashMapSearch(&s->tableDirectory.mTableRecords, {"loca"});
    assert(fLoca);

    u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    const auto& table = *fLoca.pData;
    u32 offset = NPOS;

    if (s->head.indexToLocFormat == 1)
    {
        s->p.pos = table.offset + idx*4;
        offset = BinRead32Rev(&s->p);
    }
    else
    {
        s->p.pos = table.offset + idx*2;
        offset = BinRead16Rev(&s->p);
    }

    return offset;
}

static void
FontParse(Font* s)
{
    LOG_GOOD("loading font: '%.*s'\n", s->p.sPath.size, s->p.sPath.pData);

    if (s->p.sFile.size == 0) LOG_FATAL("unable to parse empty file\n");

    auto& td = s->tableDirectory;

    td.sfntVersion = BinRead32Rev(&s->p);
    td.numTables = BinRead16Rev(&s->p);
    td.searchRange = BinRead16Rev(&s->p);
    td.entrySelector = BinRead16Rev(&s->p);
    td.rangeShift = BinRead16Rev(&s->p);

    if (td.sfntVersion != 0x00010000 && td.sfntVersion != 0x4f54544f)
        LOG_FATAL("Unable to read this ('%.*s') ttf header: sfntVersion: %u'\n", s->p.sPath.size, s->p.sPath.pData, td.sfntVersion);

#ifdef D_TTF
    u16 _searchRangeCheck = pow(2, floor(log2(td.numTables))) * 16;
    assert(td.searchRange == _searchRangeCheck);

    u16 _entrySelectorCheck = (u16)floor(log2(td.numTables));
    assert(td.entrySelector == _entrySelectorCheck);

    u16 _rangeShiftCheck = td.numTables*16 - td.searchRange;
    assert(td.rangeShift == _rangeShiftCheck);

    LOG(
        "sfntVersion: %u, numTables: %u, searchRange: %u, entrySelector: %u, rangeShift: %u\n",
        td.sfntVersion, td.numTables, td.searchRange, td.entrySelector, td.rangeShift
    );
#endif

    auto& map = td.mTableRecords;
    map = HashMapBase<TableRecord>(s->p.pA, td.numTables * HASHMAP_DEFAULT_LOAD_FACTOR_INV);

    for (u32 i = 0; i < td.numTables; i++)
    {
        TableRecord r {
            .tag = BinReadString(&s->p, 4),
            .checkSum = BinRead32Rev(&s->p),
            .offset = BinRead32Rev(&s->p),
            .length = BinRead32Rev(&s->p),
        };

        HashMapInsert(&td.mTableRecords, s->p.pA, r);
        if (r.tag != "head")
        {
            auto checkSum = getTableChecksum((u32*)(&s->p.sFile[r.offset]), r.length);
            if (r.checkSum - checkSum != 0)
                LOG_WARN("checkSums don't match: expected: %u, got: %u\n", r.checkSum, checkSum);
        }

#ifdef D_TTF
        LOG(
            "(%u): tag: '%.*s'(%u), checkSum: %u, offset: %u, length: %u\n",
            i, r.tag.size, r.tag.pData, *(u32*)(r.tag.pData), r.checkSum, r.offset, r.length
        );
#endif
    }

#ifdef D_TTF
    auto fGlyf = HashMapSearch(&map, {"glyf"});
    if (fGlyf)
    {
        LOG(
            "glyf found: '%.*s', offset: %u, length: %u\n",
            fGlyf.pData->tag.size, fGlyf.pData->tag.pData, fGlyf.pData->offset, fGlyf.pData->length
        );
    }
    else LOG_FATAL("'glyf' header not found\n");

    auto fHead = HashMapSearch(&map, {"head"});
    if (fHead)
    {
        LOG(
            "head found: '%.*s', offset: %u, length: %u\n",
            fHead.pData->tag.size, fHead.pData->tag.pData, fHead.pData->offset, fHead.pData->length
        );
    }
    else LOG_FATAL("'head' header not found?\n");
#endif

    readHeadTable(s);
}

void
FontDestroy(Font* s)
{
    // TODO:
}

} /* namespace ttf */
} /* namespace parser */
