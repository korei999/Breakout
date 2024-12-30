#include "ttf.hh"

#include <cmath>

namespace reader
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
FontLoadParse(Font* s, String path)
{
    auto bSuc = s->p.load(path);
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

static inline MapResult<String, TableRecord>
getTable(Font* s, String sTableTag)
{
    return s->tableDirectory.mStringToTableRecord.search(sTableTag);
}

static inline FWord
readFWord(Font* s)
{
    return s->p.read16Rev();
}

static void
readHeadTable(Font* s)
{
    const u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    auto fHead = getTable(s, "head");
    assert(fHead);

    s->p.m_pos = fHead.pData->val.offset;

    auto& h = s->head;

    h.version.l = s->p.read16Rev();
    h.version.r = s->p.read16Rev();
    h.fontRevision.l = s->p.read16Rev();
    h.fontRevision.r = s->p.read16Rev();
    h.checkSumAdjustment = s->p.read32Rev();
    h.magicNumber = s->p.read32Rev();
    h.flags = s->p.read16Rev();
    h.unitsPerEm = s->p.read16Rev();
    h.created = s->p.read64Rev();
    h.modified = s->p.read64Rev();
    h.xMin = s->p.read16Rev();
    h.yMin = s->p.read16Rev();
    h.xMax = s->p.read16Rev();
    h.yMax = s->p.read16Rev();
    h.macStyle = s->p.read16Rev();
    h.lowestRecPPEM = s->p.read16Rev();
    h.fontDirectionHint = s->p.read16Rev();
    h.indexToLocFormat = s->p.read16Rev();
    h.glyphDataFormat = s->p.read16Rev();

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
    u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    auto& c = s->cmapF4;

    c.segCountX2 = s->p.read16Rev();
    c.searchRange = s->p.read16Rev();
    c.entrySelector = s->p.read16Rev();
    c.rangeShift = s->p.read16Rev();

    auto segCount = c.segCountX2 / 2;
    c.mCodeToGlyphIdx = {s->p.m_pAlloc, u32(segCount)};

    auto searchRangeCheck = 2*(std::pow(2, std::floor(std::log2(segCount))));
    assert(c.searchRange == searchRangeCheck);

    /* just set pointer and skip bytes, swap bytes after */
    c.endCode = (u16*)&s->p.m_sFile[s->p.m_pos];
    s->p.m_pos += c.segCountX2;

    assert(c.endCode[segCount - 1] == 0xffff);

    c.reservedPad = s->p.read16Rev();
    assert(c.reservedPad == 0);

    c.startCode = (u16*)&s->p.m_sFile[s->p.m_pos];
    s->p.m_pos += c.segCountX2;
    assert(c.startCode[segCount - 1] == 0xffff);

    c.idDelta = (u16*)&s->p.m_sFile[s->p.m_pos];
    s->p.m_pos += c.segCountX2;

    c.idRangeOffset = (u16*)&s->p.m_sFile[s->p.m_pos];
    s->p.m_pos += c.segCountX2;

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
    u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    s->p.m_pos = offset;

    u16 format = s->p.read16Rev();
    u16 length = s->p.read16Rev();
    u16 language = s->p.read16Rev();

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
    const u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    auto fCmap = getTable(s, "cmap");
    assert(fCmap);

    s->p.m_pos = fCmap.pData->val.offset;

    auto& c = s->cmap;

    c.version = s->p.read16Rev();
    assert(c.version == 0 && "they say it's set to zero");
    c.numberSubtables = s->p.read16Rev();

    c.aSubtables = {s->p.m_pAlloc, c.numberSubtables};

    for (u32 i = 0; i < c.numberSubtables; ++i)
    {
        c.aSubtables.push(s->p.m_pAlloc, {
            .platformID = s->p.read16Rev(),
            .platformSpecificID = s->p.read16Rev(),
            .offset = s->p.read32Rev(),
        });

        const auto& lastSt = c.aSubtables.last();

#ifdef D_TTF
        LOG_NOTIFY(
            "readCmap: platformID: {}('{}'), platformSpecificID: {}('{}')\n",
            lastSt.platformID, platformIDToString(lastSt.platformID),
            lastSt.platformSpecificID, platformSpecificIDToString(lastSt.platformSpecificID)
        );
#endif
        if (lastSt.platformID == 3 && lastSt.platformSpecificID <= 1)
        {
            readCmap(s, fCmap.pData->val.offset + lastSt.offset);
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
    const u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    auto fLoca = getTable(s, "loca");
    assert(fLoca);
    const auto& locaTable = *fLoca.pData;

    u32 offset = NPOS;

    if (s->head.indexToLocFormat == 1)
    {
        s->p.m_pos = locaTable.val.offset + idx*4;
        offset = s->p.read32Rev();
    }
    else
    {
        s->p.m_pos = locaTable.val.offset + idx*2;
        offset = s->p.read16Rev();
    }

    auto fGlyf = getTable(s, "glyf");
    assert(fGlyf);

    return offset + fGlyf.pData->val.offset;
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
        sg.aEndPtsOfContours.push(s->p.m_pAlloc, s->p.read16Rev());

    /* skip instructions */
    sg.instructionLength = s->p.read16Rev();
    s->p.m_pos += sg.instructionLength;

    if (g->numberOfContours == 0)
        return;

    u32 numPoints = sg.aEndPtsOfContours.last() + 1;

    for (u32 i = 0; i < numPoints; i++)
    {
        OUTLINE_FLAG eFlag = OUTLINE_FLAG(s->p.read8());
        sg.aeFlags.push(s->p.m_pAlloc, eFlag);
        sg.aPoints.push(s->p.m_pAlloc, {
            .bOnCurve = bool(eFlag & ON_CURVE)
        });

        if (eFlag & REPEAT)
        {
            u32 repeatCount = s->p.read8();
            assert(repeatCount > 0);

            i += repeatCount;
            while (repeatCount-- > 0)
            {
                sg.aeFlags.push(s->p.m_pAlloc, eFlag);
                sg.aPoints.push(s->p.m_pAlloc, {
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
                    val += s->p.read8();
                else
                    val -= s->p.read8();
            }
            else if (~eFlag & eDelta)
            {
                val += s->p.read16Rev();
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
    auto fIdx = c.mCodeToGlyphIdx.search(code);

    if (fIdx) return fIdx.pData->val;

    u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

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
                s->p.m_pos = glyphIndexAddr;
                idx = s->p.read16Rev();
            }
            else idx = (std::byteswap(c.idDelta[i]) + code) & 0xffff;

            c.mCodeToGlyphIdx.insert(s->p.m_pAlloc, code, u16(idx));
            break;
        }
    }

    return idx;
}

Glyph
FontReadGlyph(Font* s, u32 code)
{
    const u32 savedPos = s->p.m_pos;
    defer(s->p.m_pos = savedPos);

    const auto glyphIdx = getGlyphIdx(s, code);
    const u32 offset = getGlyphOffset(s, glyphIdx);

    auto fCachedGlyph = s->mOffsetToGlyph.search(offset);
    if (fCachedGlyph) return fCachedGlyph.pData->val;

    const auto fGlyf = getTable(s, "glyf");
    const auto& glyfTable = *fGlyf.pData;

    assert(fGlyf);

    assert(offset >= glyfTable.val.offset);

    if (offset >= glyfTable.val.offset + glyfTable.val.length)
        return {{}, false};

    s->p.m_pos = offset;
    Glyph g {
        .numberOfContours = s16(s->p.read16Rev()),
        .xMin = readFWord(s),
        .yMin = readFWord(s),
        .xMax = readFWord(s),
        .yMax = readFWord(s),
    };

    assert(g.numberOfContours >= -1);

    if (g.numberOfContours == -1)
        readCompoundGlyph(s, &g);
    else readSimpleGlyph(s, &g);

    s->mOffsetToGlyph.insert(s->p.m_pAlloc, offset, g);

    return g;
};

void
FontPrintGlyphDBG(Font* s, const Glyph& g, bool bNormalize)
{
    auto& sg = g.uGlyph.simple;
    COUT("xMin: {}, yMin: {}, xMax: {}, yMax: {}\n", g.xMin, g.yMin, g.xMax, g.yMax);
    COUT(
        "instructionLength: {}, points: {}, numberOfContours: {}, aEndPtsOfContours.size: {}\n",
        sg.instructionLength, sg.aPoints.getSize(), g.numberOfContours, sg.aEndPtsOfContours.getSize()
    );

    for (auto& cn : sg.aEndPtsOfContours)
    {
        u32 idx = sg.aEndPtsOfContours.idx(&cn);
        COUT("cn({}): {}", idx, cn);
        if (idx != sg.aEndPtsOfContours.getSize() - 1) COUT(", ");
        else COUT(" ");
    }
    COUT("\n");

    if (bNormalize)
    {
        for (auto& e : sg.aPoints)
        {
            u32 i = sg.aPoints.idx(&e);
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
    LOG_GOOD("loading font: '{}'\n", s->p.m_sPath);

    if (s->p.m_sFile.getSize() == 0) LOG_FATAL("unable to parse empty file\n");

    auto& td = s->tableDirectory;

    td.sfntVersion = s->p.read32Rev();
    td.numTables = s->p.read16Rev();
    td.searchRange = s->p.read16Rev();
    td.entrySelector = s->p.read16Rev();
    td.rangeShift = s->p.read16Rev();

    if (td.sfntVersion != 0x00010000 && td.sfntVersion != 0x4f54544f)
        LOG_FATAL("Unable to read this ('{}') ttf header: sfntVersion: {}'\n", s->p.m_sPath, td.sfntVersion);

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

    auto& map = td.mStringToTableRecord;
    map = MapBase<String, TableRecord>(s->p.m_pAlloc, td.numTables * MAP_DEFAULT_LOAD_FACTOR_INV);

    for (u32 i = 0; i < td.numTables; i++)
    {
        TableRecord r {
            .tag = s->p.readString(4),
            .checkSum = s->p.read32Rev(),
            .offset = s->p.read32Rev(),
            .length = s->p.read32Rev(),
        };

        td.mStringToTableRecord.insert(s->p.m_pAlloc, r.tag, r);
        if (r.tag != "head")
        {
            auto checkSum = getTableChecksum((u32*)(&s->p.m_sFile[r.offset]), r.length);
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
} /* namespace reader */
