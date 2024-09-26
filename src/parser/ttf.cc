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
    if (!bSuc) LOG_FATAL("BinLoadFile failed: '{}'\n", path);

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

inline HashMapResult<TableRecord>
getTable(Font* s, String sTableTag)
{
    return HashMapSearch(&s->tableDirectory.mTableRecords, {sTableTag});
}

inline FWord
readFWord(Font* s)
{
    return BinRead16Rev(&s->p);
}

static void
readHeadTable(Font* s)
{
    const u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    auto fHead = getTable(s, "head");
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
        "\t\tversion: {}, {}\n"
        "\t\tfontRevision: {}, {}\n"
        "\t\tcheckSumAdjustment: {}\n"
        "\t\tmagicNumber: {}\n"
        "\t\tflags: {:#x}\n"
        "\t\tunitsPerEm: {}\n"
        "\t\tcreated: {}\n"
        "\t\tmodified: {}\n"
        "\t\txMin: {}\n"
        "\t\tyMin: {}\n"
        "\t\txMax: {}\n"
        "\t\tyMax: {}\n"
        "\t\tmacStyle: {}\n"
        "\t\tlowestRecPPEM: {}\n"
        "\t\tfontDirectionHint: {}\n"
        "\t\tindexToLocFormat: {}\n"
        "\t\tglyphDataFormat: {}\n",
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
    const u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    auto fLoca = getTable(s, "loca");
    assert(fLoca);
    const auto& locaTable = *fLoca.pData;

    u32 offset = NPOS;

    if (s->head.indexToLocFormat == 1)
    {
        s->p.pos = locaTable.offset + idx*4;
        offset = BinRead32Rev(&s->p);
    }
    else
    {
        s->p.pos = locaTable.offset + idx*2;
        offset = BinRead16Rev(&s->p);
    }

    auto fGlyf = getTable(s, "glyf");
    assert(fGlyf);

    return offset + fGlyf.pData->offset;
}

static void
readCompoundGlyph(Font* s, Glyph* g)
{
    // TODO:
    LOG_WARN("ignoring compound glyph...\n");
}

static void
readSimpleGlyph(Font* s, Glyph* g)
{
    auto& sg = g->uGlyph.simple;

    LOG_GOOD("Reading simple glyph...\n");

    sg.aEndPtsOfContours = {s->p.pAlloc, g->numberOfContours};
    for (s16 i = 0; i < g->numberOfContours; i++)
        VecPush(&sg.aEndPtsOfContours, s->p.pAlloc, BinRead16Rev(&s->p));

    /* skip instructions */
    sg.instructionLength = BinRead16Rev(&s->p);
    s->p.pos += sg.instructionLength;

    if (g->numberOfContours == 0)
        return;

    u32 numPoints = utils::searchMax(sg.aEndPtsOfContours) + 1;

    for (u32 i = 0; i < numPoints; i++)
    {
        OUTLINE_FLAG eFlag = OUTLINE_FLAG(BinRead8(&s->p));
        VecPush(&sg.aeFlags, s->p.pAlloc, eFlag);
        VecPush(&sg.aPoints, s->p.pAlloc, {
            .bOnCurve = eFlag & ON_CURVE
        });

        if (eFlag & REPEAT)
        {
            u32 repeatCount = BinRead8(&s->p);
            assert(repeatCount > 0);

            i += repeatCount;
            while (repeatCount-- > 0)
            {
                VecPush(&sg.aeFlags, s->p.pAlloc, eFlag);
                VecPush(&sg.aPoints, s->p.pAlloc, {
                    .bOnCurve = eFlag & ON_CURVE
                });
            }
        }
    }

    auto readCoords = [&](bool bXorY, OUTLINE_FLAG eByte, OUTLINE_FLAG eDelta) -> void {
        s16 val = 0;

        for (u32 i = 0; i < numPoints; i++)
        {
            auto eFlag = sg.aeFlags[i];
            if (eFlag & eByte)
            {
                if (eFlag & eDelta)
                    val += BinRead8(&s->p);
                else
                    val -= BinRead8(&s->p);
            }
            else if (~eFlag & eDelta)
            {
                val += BinRead16Rev(&s->p);
            }

            if (bXorY) sg.aPoints[i].x = val;
            else sg.aPoints[i].y = val;
        }
    };

    readCoords(true, X_SHORT_VECTOR, THIS_X_IS_SAME);
    readCoords(false, Y_SHORT_VECTOR, THIS_Y_IS_SAME);
}

Option<Glyph>
FontReadGlyph(Font* s, u32 idx)
{
    const u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    const u32 offset = getGlyphOffset(s, idx);
    const auto fGlyf = getTable(s, "glyf");
    const auto& glyfTable = *fGlyf.pData;

    assert(fGlyf);

    LOG("offset: {}, glyfTable.offset: {}\n", offset, glyfTable.offset);
    assert(offset >= glyfTable.offset);

    if (offset >= glyfTable.offset + glyfTable.length)
        return {{}, false};

    s->p.pos = offset;
    Glyph g {
        .numberOfContours = BinRead16Rev(&s->p),
        .xMin = readFWord(s),
        .yMin = readFWord(s),
        .xMax = readFWord(s),
        .yMax = readFWord(s),
    };

    assert(g.numberOfContours >= - 1);

    if (g.numberOfContours == -1)
        readCompoundGlyph(s, &g);
    else readSimpleGlyph(s, &g);

    return g;
};

void
FontPrintGlyph(Font* s, Glyph* g)
{
    auto& sg = g->uGlyph.simple;
    COUT("xMin: {}, yMin: {}, xMax: {}, yMax: {}\n", g->xMin, g->yMin, g->xMax, g->yMax);
    COUT("instructionLength: {}, points: {}\n", sg.instructionLength, VecSize(&sg.aPoints));
    for (auto& e : sg.aPoints)
        COUT("x: {}, y: {}\n", e.x, e.y);
}

static void
FontParse(Font* s)
{
    LOG_GOOD("loading font: '{}'\n", s->p.sPath);

    if (s->p.sFile.size == 0) LOG_FATAL("unable to parse empty file\n");

    auto& td = s->tableDirectory;

    td.sfntVersion = BinRead32Rev(&s->p);
    td.numTables = BinRead16Rev(&s->p);
    td.searchRange = BinRead16Rev(&s->p);
    td.entrySelector = BinRead16Rev(&s->p);
    td.rangeShift = BinRead16Rev(&s->p);

    if (td.sfntVersion != 0x00010000 && td.sfntVersion != 0x4f54544f)
        LOG_FATAL("Unable to read this ('{}') ttf header: sfntVersion: {}'\n", s->p.sPath, td.sfntVersion);

#ifdef D_TTF
    u16 _searchRangeCheck = pow(2, floor(log2(td.numTables))) * 16;
    assert(td.searchRange == _searchRangeCheck);

    u16 _entrySelectorCheck = (u16)floor(log2(td.numTables));
    assert(td.entrySelector == _entrySelectorCheck);

    u16 _rangeShiftCheck = td.numTables*16 - td.searchRange;
    assert(td.rangeShift == _rangeShiftCheck);

    LOG(
        "sfntVersion: {}, numTables: {}, searchRange: {}, entrySelector: {}, rangeShift: {}\n",
        td.sfntVersion, td.numTables, td.searchRange, td.entrySelector, td.rangeShift
    );
#endif

    auto& map = td.mTableRecords;
    map = HashMapBase<TableRecord>(s->p.pAlloc, td.numTables * HASHMAP_DEFAULT_LOAD_FACTOR_INV);

    for (u32 i = 0; i < td.numTables; i++)
    {
        TableRecord r {
            .tag = BinReadString(&s->p, 4),
            .checkSum = BinRead32Rev(&s->p),
            .offset = BinRead32Rev(&s->p),
            .length = BinRead32Rev(&s->p),
        };

        HashMapInsert(&td.mTableRecords, s->p.pAlloc, r);
        if (r.tag != "head")
        {
            auto checkSum = getTableChecksum((u32*)(&s->p.sFile[r.offset]), r.length);
            if (r.checkSum - checkSum != 0)
                LOG_WARN("checkSums don't match: expected: {}, got: {}\n", r.checkSum, checkSum);
        }

#ifdef D_TTF
        LOG(
            "({}): tag: '{}'({}), checkSum: {}, offset: {}, length: {}\n",
            i, r.tag, *(u32*)(r.tag.pData), r.checkSum, r.offset, r.length
        );
#endif
    }

#ifdef D_TTF
    auto fGlyf = getTable(s, "glyf");
    if (fGlyf)
    {
        LOG(
            "'glyf' found: '{}', offset: {}, length: {}\n",
            fGlyf.pData->tag, fGlyf.pData->offset, fGlyf.pData->length
        );
    }
    else LOG_FATAL("'glyf' header not found\n");

    auto fHead = getTable(s, "head");
    if (fHead)
    {
        LOG(
            "'head' found: '{}', offset: {}, length: {}\n",
            fHead.pData->tag, fHead.pData->offset, fHead.pData->length
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
