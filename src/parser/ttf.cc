#include "ttf.hh"

#include <cmath>

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
FontLoadAndParse(Font* s, String path)
{
    auto bSuc = BinLoadFile(&s->p, path);
    if (!bSuc) LOG_FATAL("BinLoadFile failed: '{}'\n", path);

    FontParse(s);

    /* cache some ascii glyphs */
    for (u32 i = 21; i < 127; ++i)
        FontReadGlyph(s, i);

    return true;
}

constexpr u32
getTableChecksum(u32* pTable, u32 nBytes)
{
    u32 sum = 0;
    u32 nLongs = (nBytes + 3) / 4;
    while (nLongs > 0)
    {
        sum += std::byteswap(*pTable);
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

inline MapResult<TableRecord>
getTable(Font* s, String sTableTag)
{
    return MapSearch(&s->tableDirectory.mTableRecords, {sTableTag});
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

#ifdef D_TTF
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
#endif
}

static void
readCmapFormat4(Font* s)
{
    u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    auto& c = s->cmapF4;

    c.segCountX2 = BinRead16Rev(&s->p);
    c.searchRange = BinRead16Rev(&s->p);
    c.entrySelector = BinRead16Rev(&s->p);
    c.rangeShift = BinRead16Rev(&s->p);

    auto segCount = c.segCountX2 / 2;
    c.mGlyphIndices = {s->p.pAlloc, u32(segCount)};

    auto searchRangeCheck = 2*(pow(2, std::floor(log2(segCount))));
    assert(c.searchRange == searchRangeCheck);

    /* just set pointer and skip bytes, swap bytes after */
    c.endCode = (u16*)&s->p.sFile[s->p.pos];
    s->p.pos += c.segCountX2;

    assert(c.endCode[segCount - 1] == 0xffff);

    c.reservedPad = BinRead16Rev(&s->p);
    assert(c.reservedPad == 0);

    c.startCode = (u16*)&s->p.sFile[s->p.pos];
    s->p.pos += c.segCountX2;
    assert(c.startCode[segCount - 1] == 0xffff);

    c.idDelta = (u16*)&s->p.sFile[s->p.pos];
    s->p.pos += c.segCountX2;

    c.idRangeOffset = (u16*)&s->p.sFile[s->p.pos];
    s->p.pos += c.segCountX2;

#ifdef D_TTF
    LOG_NOTIFY(
        "\treadCmapFormat4:\n"
        "\t\tformat: {}\n"
        "\t\tlength: {}\n"
        "\t\tlanguage: {}\n"
        "\t\tsegCountX2: {}\n"
        "\t\tsearchRange: {}, (check: {})\n"
        "\t\tentrySelector: {}\n"
        "\t\trangeShift: {}\n"
        "\t\treservedPad: {}\n",
        c.format,
        c.length,
        c.language,
        c.segCountX2,
        c.searchRange, searchRangeCheck,
        c.entrySelector,
        c.rangeShift,
        c.reservedPad
    );
#endif
}

static void
readCmap(Font* s, u32 offset)
{
    u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    s->p.pos = offset;

    u16 format = BinRead16Rev(&s->p);
    u16 length = BinRead16Rev(&s->p);
    u16 language = BinRead16Rev(&s->p);

#ifdef D_TTF
    LOG_NOTIFY("readCmap: format: {}, length: {}, language: {}\n", format, length, language);
#endif

    // TODO: other formats
    if (format == 4)
    {
        s->cmapF4.format = format;
        s->cmapF4.length = length;
        s->cmapF4.language = language;
        readCmapFormat4(s);
    }
}

static void
readCmapTable(Font* s)
{
    const u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    auto fCmap = getTable(s, "cmap");
    assert(fCmap);

    s->p.pos = fCmap.pData->offset;

    auto& c = s->cmap;

    c.version = BinRead16Rev(&s->p);
    assert(c.version == 0 && "they say it's set to zero");
    c.numberSubtables = BinRead16Rev(&s->p);

    c.aSubtables = {s->p.pAlloc, c.numberSubtables};

    for (u32 i = 0; i < c.numberSubtables; ++i)
    {
        VecPush(&c.aSubtables, s->p.pAlloc, {
            .platformID = BinRead16Rev(&s->p),
            .platformSpecificID = BinRead16Rev(&s->p),
            .offset = BinRead32Rev(&s->p),
        });

        const auto& lastSt = VecLast(&c.aSubtables);

#ifdef D_TTF
        LOG_NOTIFY(
            "readCmap: platformID: {}('{}'), platformSpecificID: {}('{}')\n",
            lastSt.platformID, platformIDToString(lastSt.platformID),
            lastSt.platformSpecificID, platformSpecificIDToString(lastSt.platformSpecificID)
        );
#endif
        if (lastSt.platformID == 3 && lastSt.platformSpecificID <= 1)
        {
            readCmap(s, fCmap.pData->offset + lastSt.offset);
            break;
        }
        else if (lastSt.platformID == 0 && lastSt.platformSpecificID == 3)
        {
            // TODO: Unicode, Unicode 2.0 or later (BMP only)
        }
    }

#ifdef D_TTF
    LOG(
        "\tcmap:\n"
        "\t\tversion: {}\n"
        "\t\tnumberSubtables: {}\n"
        "\t\tSubtables:\n",
        c.version,
        c.numberSubtables
    );
    for (auto& st : c.aSubtables)
    {
        LOG(
            "\t({}): platformID: {}('{}')\n"
            "\t      platformSpecificID: {}('{}')\n"
            "\t      offset: {}\n",
            VecIdx(&c.aSubtables, &st),
            st.platformID, platformIDToString(st.platformID),
            st.platformSpecificID, platformSpecificIDToString(st.platformSpecificID),
            st.offset
        );
    }
#endif
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

    for (s16 i = 0; i < g->numberOfContours; i++)
        VecPush(&sg.aEndPtsOfContours, s->p.pAlloc, BinRead16Rev(&s->p));

    /* skip instructions */
    sg.instructionLength = BinRead16Rev(&s->p);
    s->p.pos += sg.instructionLength;

    if (g->numberOfContours == 0)
        return;

    u32 numPoints = VecLast(&sg.aEndPtsOfContours) + 1;

    for (u32 i = 0; i < numPoints; i++)
    {
        OUTLINE_FLAG eFlag = OUTLINE_FLAG(BinRead8(&s->p));
        VecPush(&sg.aeFlags, s->p.pAlloc, eFlag);
        VecPush(&sg.aPoints, s->p.pAlloc, {
            .bOnCurve = bool(eFlag & ON_CURVE)
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
                    .bOnCurve = bool(eFlag & ON_CURVE)
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

static u32
getGlyphIdx(Font* s, u16 code)
{
    auto& c = s->cmapF4;
    auto fIdx = MapSearch(&c.mGlyphIndices, {code});

    if (fIdx) return fIdx.pData->glyphIdx;

    u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    u32 idx = 0, glyphIndexAddr = 0;

    for (u16 i = 0; i < c.segCountX2/2; ++i)
    {
        if (std::byteswap(c.startCode[i]) <= code && std::byteswap(c.endCode[i]) >= code)
        {
            idx = 0, glyphIndexAddr = 0;
            if (std::byteswap(c.idRangeOffset[i]))
            {
                glyphIndexAddr = std::byteswap(c.idRangeOffset[i]) +
                    2 * (code - std::byteswap(c.startCode[i]));
                s->p.pos = glyphIndexAddr;
                idx = BinRead16Rev(&s->p);
            }
            else idx = (std::byteswap(c.idDelta[i]) + code) & 0xffff;

            MapInsert(&c.mGlyphIndices, s->p.pAlloc, {code, u16(idx)});
            break;
        }
    }

    return idx;
}

Glyph
FontReadGlyph(Font* s, u32 code)
{
    const u32 savedPos = s->p.pos;
    defer(s->p.pos = savedPos);

    const auto glyphIdx = getGlyphIdx(s, code);
    const u32 offset = getGlyphOffset(s, glyphIdx);

    auto fCachedGlyph = MapSearch(&s->mOffsetToGlyph, {offset});
    if (fCachedGlyph) return fCachedGlyph.pData->glyph;

    const auto fGlyf = getTable(s, "glyf");
    const auto& glyfTable = *fGlyf.pData;

    assert(fGlyf);

    assert(offset >= glyfTable.offset);

    if (offset >= glyfTable.offset + glyfTable.length)
        return {{}, false};

    s->p.pos = offset;
    Glyph g {
        .numberOfContours = s16(BinRead16Rev(&s->p)),
        .xMin = readFWord(s),
        .yMin = readFWord(s),
        .xMax = readFWord(s),
        .yMax = readFWord(s),
    };

    assert(g.numberOfContours >= - 1);

    if (g.numberOfContours == -1)
        readCompoundGlyph(s, &g);
    else readSimpleGlyph(s, &g);

    MapInsert(&s->mOffsetToGlyph, s->p.pAlloc, {offset, g});

    return g;
};

void
FontPrintGlyph(Font* s, const Glyph& g, bool bNormalize)
{
    auto& sg = g.uGlyph.simple;
    COUT("xMin: {}, yMin: {}, xMax: {}, yMax: {}\n", g.xMin, g.yMin, g.xMax, g.yMax);
    COUT(
        "instructionLength: {}, points: {}, numberOfContours: {}, aEndPtsOfContours.size: {}\n",
        sg.instructionLength, VecSize(&sg.aPoints), g.numberOfContours, VecSize(&sg.aEndPtsOfContours)
    );

    for (auto& cn : sg.aEndPtsOfContours)
    {
        u32 idx = VecIdx(&sg.aEndPtsOfContours, &cn);
        COUT("cn({}): {}", idx, cn);
        if (idx != VecSize(&sg.aEndPtsOfContours) - 1) COUT(", ");
        else COUT(" ");
    }
    COUT("\n");

    if (bNormalize)
    {
        for (auto& e : sg.aPoints)
        {
            u32 i = VecIdx(&sg.aPoints, &e);
            COUT(
                "({}): x: {}, y: {}, bOnCurve: {}\n",
                i, f32(e.x) / f32(g.xMax), f32(e.y) / f32(g.yMax),
                e.bOnCurve
            );
        }
    }
    else
    {
        for (auto& e : sg.aPoints)
            COUT("x: {}, y: {}, bOnCurve: {}\n", e.x, e.y, e.bOnCurve);
    }
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
    u16 _searchRangeCheck = pow(2, std::floor(log2(td.numTables))) * 16;
    assert(td.searchRange == _searchRangeCheck);

    u16 _entrySelectorCheck = (u16)std::floor(log2(td.numTables));
    assert(td.entrySelector == _entrySelectorCheck);

    u16 _rangeShiftCheck = td.numTables*16 - td.searchRange;
    assert(td.rangeShift == _rangeShiftCheck);

    LOG(
        "sfntVersion: {}, numTables: {}, searchRange: {}, entrySelector: {}, rangeShift: {}\n",
        td.sfntVersion, td.numTables, td.searchRange, td.entrySelector, td.rangeShift
    );
#endif

    auto& map = td.mTableRecords;
    map = MapBase<TableRecord>(s->p.pAlloc, td.numTables * MAP_DEFAULT_LOAD_FACTOR_INV);

    for (u32 i = 0; i < td.numTables; i++)
    {
        TableRecord r {
            .tag = BinReadString(&s->p, 4),
            .checkSum = BinRead32Rev(&s->p),
            .offset = BinRead32Rev(&s->p),
            .length = BinRead32Rev(&s->p),
        };

        MapInsert(&td.mTableRecords, s->p.pAlloc, r);
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
    readCmapTable(s);
}

void
FontDestroy(Font* s)
{
    // TODO:
}

} /* namespace ttf */
} /* namespace parser */
