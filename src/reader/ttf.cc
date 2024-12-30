#include "ttf.hh"

#include <cmath>

namespace reader
{
namespace ttf
{

constexpr f32
F2Dot14Tof32(F2Dot14 x)
{
    f32 ret = f32((0xc000 & x) >> 14);
    ret += (f32(x & 0x3fff)) / f32(0x3fff);

    return ret;
}

bool
Font::loadParse(String path)
{
    auto bSuc = m_bin.load(path);
    if (!bSuc) LOG_FATAL("BinLoadFile failed: '{}'\n", path);

    parse();

    /* cache some ascii glyphs */
    for (u32 i = 21; i < 127; ++i)
        readGlyph(i);

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

MapResult<String, TableRecord>
Font::getTable(String sTableTag)
{
    return m_tableDirectory.mStringToTableRecord.search(sTableTag);
}

FWord
Font::readFWord()
{
    return m_bin.read16Rev();
}

void
Font::readHeadTable()
{
    const u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    auto fHead = getTable("head");
    assert(fHead);

    m_bin.m_pos = fHead.pData->val.offset;

    auto& h = m_head;

    h.version.l = m_bin.read16Rev();
    h.version.r = m_bin.read16Rev();
    h.fontRevision.l = m_bin.read16Rev();
    h.fontRevision.r = m_bin.read16Rev();
    h.checkSumAdjustment = m_bin.read32Rev();
    h.magicNumber = m_bin.read32Rev();
    h.flags = m_bin.read16Rev();
    h.unitsPerEm = m_bin.read16Rev();
    h.created = m_bin.read64Rev();
    h.modified = m_bin.read64Rev();
    h.xMin = m_bin.read16Rev();
    h.yMin = m_bin.read16Rev();
    h.xMax = m_bin.read16Rev();
    h.yMax = m_bin.read16Rev();
    h.macStyle = m_bin.read16Rev();
    h.lowestRecPPEM = m_bin.read16Rev();
    h.fontDirectionHint = m_bin.read16Rev();
    h.indexToLocFormat = m_bin.read16Rev();
    h.glyphDataFormat = m_bin.read16Rev();

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

void
Font::readCmapFormat4()
{
    u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    auto& c = m_cmapF4;

    c.segCountX2 = m_bin.read16Rev();
    c.searchRange = m_bin.read16Rev();
    c.entrySelector = m_bin.read16Rev();
    c.rangeShift = m_bin.read16Rev();

    auto segCount = c.segCountX2 / 2;
    c.mCodeToGlyphIdx = {m_bin.m_pAlloc, u32(segCount)};

    auto searchRangeCheck = 2*(std::pow(2, std::floor(std::log2(segCount))));
    assert(c.searchRange == searchRangeCheck);

    /* just set pointer and skip bytes, swap bytes after */
    c.endCode = (u16*)&m_bin[m_bin.m_pos];
    m_bin.m_pos += c.segCountX2;

    assert(c.endCode[segCount - 1] == 0xffff);

    c.reservedPad = m_bin.read16Rev();
    assert(c.reservedPad == 0);

    c.startCode = (u16*)&m_bin[m_bin.m_pos];
    m_bin.m_pos += c.segCountX2;
    assert(c.startCode[segCount - 1] == 0xffff);

    c.idDelta = (u16*)&m_bin[m_bin.m_pos];
    m_bin.m_pos += c.segCountX2;

    c.idRangeOffset = (u16*)&m_bin[m_bin.m_pos];
    m_bin.m_pos += c.segCountX2;

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

void
Font::readCmap(u32 offset)
{
    u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    m_bin.m_pos = offset;

    u16 format = m_bin.read16Rev();
    u16 length = m_bin.read16Rev();
    u16 language = m_bin.read16Rev();

#ifdef D_TTF
    LOG_NOTIFY("readCmap: format: {}, length: {}, language: {}\n", format, length, language);
#endif

    // TODO: other formats
    if (format == 4)
    {
        m_cmapF4.format = format;
        m_cmapF4.length = length;
        m_cmapF4.language = language;
        readCmapFormat4();
    }
}

void
Font::readCmapTable()
{
    const u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    auto fCmap = getTable("cmap");
    assert(fCmap);

    m_bin.m_pos = fCmap.pData->val.offset;

    auto& c = m_cmap;

    c.version = m_bin.read16Rev();
    assert(c.version == 0 && "they say it's set to zero");
    c.numberSubtables = m_bin.read16Rev();

    c.aSubtables = {m_bin.m_pAlloc, c.numberSubtables};

    for (u32 i = 0; i < c.numberSubtables; ++i)
    {
        c.aSubtables.push(m_bin.m_pAlloc, {
            .platformID = m_bin.read16Rev(),
            .platformSpecificID = m_bin.read16Rev(),
            .offset = m_bin.read32Rev(),
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
            readCmap(fCmap.pData->val.offset + lastSt.offset);
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

u32
Font::getGlyphOffset(u32 idx)
{
    const u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    auto fLoca = getTable("loca");
    assert(fLoca);
    const auto& locaTable = *fLoca.pData;

    u32 offset = NPOS;

    if (m_head.indexToLocFormat == 1)
    {
        m_bin.m_pos = locaTable.val.offset + idx*4;
        offset = m_bin.read32Rev();
    }
    else
    {
        m_bin.m_pos = locaTable.val.offset + idx*2;
        offset = m_bin.read16Rev();
    }

    auto fGlyf = getTable("glyf");
    assert(fGlyf);

    return offset + fGlyf.pData->val.offset;
}

void
Font::readCompoundGlyph(Glyph* g)
{
    // TODO:
    LOG_WARN("ignoring compound glyph...\n");
}

void
Font::readSimpleGlyph(Glyph* g)
{
    auto& sg = g->uGlyph.simple;

    for (s16 i = 0; i < g->numberOfContours; i++)
        sg.aEndPtsOfContours.push(m_bin.m_pAlloc, m_bin.read16Rev());

    /* skip instructions */
    sg.instructionLength = m_bin.read16Rev();
    m_bin.m_pos += sg.instructionLength;

    if (g->numberOfContours == 0)
        return;

    u32 numPoints = sg.aEndPtsOfContours.last() + 1;

    for (u32 i = 0; i < numPoints; i++)
    {
        OUTLINE_FLAG eFlag = OUTLINE_FLAG(m_bin.read8());
        sg.aeFlags.push(m_bin.m_pAlloc, eFlag);
        sg.aPoints.push(m_bin.m_pAlloc, {
            .bOnCurve = bool(eFlag & ON_CURVE)
        });

        if (eFlag & REPEAT)
        {
            u32 repeatCount = m_bin.read8();
            assert(repeatCount > 0);

            i += repeatCount;
            while (repeatCount-- > 0)
            {
                sg.aeFlags.push(m_bin.m_pAlloc, eFlag);
                sg.aPoints.push(m_bin.m_pAlloc, {
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
                    val += m_bin.read8();
                else
                    val -= m_bin.read8();
            }
            else if (~eFlag & eDelta)
            {
                val += m_bin.read16Rev();
            }

            if (bXorY) sg.aPoints[i].x = val;
            else sg.aPoints[i].y = val;
        }
    };

    readCoords(true, X_SHORT_VECTOR, THIS_X_IS_SAME);
    readCoords(false, Y_SHORT_VECTOR, THIS_Y_IS_SAME);
}

u32
Font::getGlyphIdx(u16 code)
{
    auto& c = m_cmapF4;
    auto fIdx = c.mCodeToGlyphIdx.search(code);

    if (fIdx) return fIdx.pData->val;

    u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

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
                m_bin.m_pos = glyphIndexAddr;
                idx = m_bin.read16Rev();
            }
            else idx = (std::byteswap(c.idDelta[i]) + code) & 0xffff;

            c.mCodeToGlyphIdx.insert(m_bin.m_pAlloc, code, u16(idx));
            break;
        }
    }

    return idx;
}

Glyph
Font::readGlyph(u32 code)
{
    const u32 savedPos = m_bin.m_pos;
    defer(m_bin.m_pos = savedPos);

    const auto glyphIdx = getGlyphIdx(code);
    const u32 offset = getGlyphOffset(glyphIdx);

    auto fCachedGlyph = m_mOffsetToGlyph.search(offset);
    if (fCachedGlyph) return fCachedGlyph.pData->val;

    const auto fGlyf = getTable("glyf");
    const auto& glyfTable = *fGlyf.pData;

    assert(fGlyf);

    assert(offset >= glyfTable.val.offset);

    if (offset >= glyfTable.val.offset + glyfTable.val.length)
        return {{}, false};

    m_bin.m_pos = offset;
    Glyph g {
        .numberOfContours = s16(m_bin.read16Rev()),
        .xMin = readFWord(),
        .yMin = readFWord(),
        .xMax = readFWord(),
        .yMax = readFWord(),
    };

    assert(g.numberOfContours >= -1);

    if (g.numberOfContours == -1)
        readCompoundGlyph(&g);
    else readSimpleGlyph(&g);

    m_mOffsetToGlyph.insert(m_bin.m_pAlloc, offset, g);

    return g;
};

void
Font::printGlyphDBG(const Glyph& g, bool bNormalize)
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

void
Font::parse()
{
    LOG_GOOD("loading font: '{}'\n", m_bin.m_sPath);

    if (m_bin.m_sFile.getSize() == 0) LOG_FATAL("unable to parse empty file\n");

    auto& td = m_tableDirectory;

    td.sfntVersion = m_bin.read32Rev();
    td.numTables = m_bin.read16Rev();
    td.searchRange = m_bin.read16Rev();
    td.entrySelector = m_bin.read16Rev();
    td.rangeShift = m_bin.read16Rev();

    if (td.sfntVersion != 0x00010000 && td.sfntVersion != 0x4f54544f)
        LOG_FATAL("Unable to read this ('{}') ttf header: sfntVersion: {}'\n", m_bin.m_sPath, td.sfntVersion);

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
    map = MapBase<String, TableRecord>(m_bin.m_pAlloc, td.numTables * MAP_DEFAULT_LOAD_FACTOR_INV);

    for (u32 i = 0; i < td.numTables; i++)
    {
        TableRecord r {
            .tag = m_bin.readString(4),
            .checkSum = m_bin.read32Rev(),
            .offset = m_bin.read32Rev(),
            .length = m_bin.read32Rev(),
        };

        td.mStringToTableRecord.insert(m_bin.m_pAlloc, r.tag, r);
        if (r.tag != "head")
        {
            auto checkSum = getTableChecksum((u32*)(&m_bin[r.offset]), r.length);
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

    readHeadTable();
    readCmapTable();
}

void
Font::destroy()
{
    // TODO:
}

} /* namespace ttf */
} /* namespace reader */