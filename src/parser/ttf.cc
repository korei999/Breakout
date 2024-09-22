#include "ttf.hh"

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
getTableChecksum(u32* Table, u32 Length)
{
    u32 Sum = 0L;
    u32 *Endptr = Table + ((Length + 3) & ~3) / sizeof(u32);
    while (Table < Endptr)
        Sum += *Table++;
    return Sum;
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
cmapRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->cmapOffset = at;
    auto& cmap = s->cmap;

    cmap.version = BinRead16Rev(&s->p);
    cmap.numberSubtables = BinRead16Rev(&s->p);

    cmap.aSubtables = Vec<CMAPEncodingSubtable>(s->p.pAlloc, cmap.numberSubtables);
    VecSetSize(&cmap.aSubtables, cmap.numberSubtables);

    LOG("\t\tversion: %#x, numberSubtables: %u\n", cmap.version, cmap.numberSubtables);

    for (auto& st : cmap.aSubtables)
    {
        st.platformID = BinRead16Rev(&s->p);
        st.platformSpecificID = BinRead16Rev(&s->p);
        st.offset = BinRead32Rev(&s->p);

        String sPlatformID = platformIDToString(st.platformID);
        String sPlatformSpecificID = platformSpecificIDToString(st.platformSpecificID);

        LOG(
            "\t\t(%u): platformID: %u(%.*s), platformSpecificID: %u(%.*s), offset(%u)\n",
            VecGetIdx(&cmap.aSubtables, &st),
            st.platformID, sPlatformID.size, sPlatformID.pData,
            st.platformSpecificID, sPlatformSpecificID.size, sPlatformSpecificID.pData,
            st.offset
        );
    }

    s->p.pos = savedPos;
}

static void
procSimpleGlyph(Font* s)
{
}

static void
glyfRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->glyphOffset = at;
    Glyph& g = s->glyph;

    g.numberOfContours = BinRead16Rev(&s->p);
    g.xMin = BinRead16Rev(&s->p);
    g.yMin = BinRead16Rev(&s->p);
    g.xMax = BinRead16Rev(&s->p);
    g.yMax = BinRead16Rev(&s->p);

    LOG(
        "\t\tnumberOfContours: %u, xMin: %u, yMin: %u, xMax: %u, yMax: %u\n",
        g.numberOfContours, g.xMin, g.yMin, g.xMax, g.yMax
    );

    if (g.numberOfContours >= 0)
    {
        procSimpleGlyph(s);
    }
    else
    {
        // TODO:
    }

    s->p.pos = savedPos;
}

static void
headRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->headOffset = at;
    Head& h = s->head;

    h.version.l = BinRead16Rev(&s->p);
    h.version.r = BinRead16Rev(&s->p);
    h.fontRevision.l = BinRead16Rev(&s->p);
    h.fontRevision.r = BinRead16Rev(&s->p);
    h.checkSumAdjustment = BinRead32Rev(&s->p);
    h.magicNumber = BinRead32Rev(&s->p);
    h.flags = BinRead16Rev(&s->p);
    h.unitsPerEm = BinRead16(&s->p);
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

    assert(h.magicNumber == 0x5f0f3cf5);

    LOG(
        "\t\tversion: (%d, %d), fontRevision: (%d, %d), checkSumAdjustment: %u\n"
        "\t\t\tmagicNumber: %#x, flags: %#x, unitsPerEm: %u, created: %lu, modified: %lu\n"
        "\t\t\txMin: %d, yMin: %d, xMax: %d, yMax: %d, macStyle: %u, lowestRecPPEM: %u\n"
        "\t\t\tfontDirectionHint: %d, indexToLocFormat: %d, glyphDataFormat: %d\n",
        h.version.l, h.version.r, h.fontRevision.l, h.fontRevision.r, h.checkSumAdjustment,
        h.magicNumber, h.flags, h.unitsPerEm, h.created, h.modified,
        h.xMin, h.yMin, h.xMax, h.yMax, h.macStyle, h.lowestRecPPEM,
        h.fontDirectionHint, h.indexToLocFormat, h.glyphDataFormat
    );

    s->p.pos = savedPos;
}

static void
hheaRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->hheaOffset = at;
    Hhea& h = s->hhea;

    h.version.l = BinRead16Rev(&s->p);
    h.version.r = BinRead16Rev(&s->p);
    h.ascent = BinRead16Rev(&s->p);
    h.descent = BinRead16Rev(&s->p);
    h.lineGap = BinRead16Rev(&s->p);
    h.advanceWidthMax = BinRead16Rev(&s->p);
    h.minLeftSideBearing = BinRead16Rev(&s->p);
    h.minRightSideBearing = BinRead16Rev(&s->p);
    h.xMaxExtent = BinRead16Rev(&s->p);
    h.caretSlopeRise = BinRead16Rev(&s->p);
    h.caretSlopeRun = BinRead16Rev(&s->p);
    h.caretOffset = BinRead16Rev(&s->p);
    h.reserved0 = BinRead16Rev(&s->p);
    h.reserved1 = BinRead16Rev(&s->p);
    h.reserved2 = BinRead16Rev(&s->p);
    h.reserved3 = BinRead16Rev(&s->p);
    h.metricDataFormat = BinRead16Rev(&s->p);
    h.numOfLongHorMetrics = BinRead16Rev(&s->p);

    LOG(
        "\t\tversion: (%u, %u), ascent: %d, descent: %d\n"
        "\t\t\tlineGap: %d, advanceWidthMax: %u, minLeftSideBearing: %d, minRightSideBearing: %d\n"
        "\t\t\txMaxExtent: %d, caretSlopeRise: %d, caretSlopeRun: %d, caretOffset: %d\n"
        "\t\t\tmetricDataFormat: %d, numOfLongHorMetrics: %u\n",
        h.version.l, h.version.r, h.ascent, h.descent,
        h.lineGap, h.advanceWidthMax, h.minLeftSideBearing, h.minRightSideBearing,
        h.xMaxExtent, h.caretSlopeRise, h.caretSlopeRun, h.caretOffset,
        h.metricDataFormat, h.numOfLongHorMetrics
    );

    s->p.pos = savedPos;
}

static void
hmtxRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->hmtxOffset = at;
    Hmtx& h = s->hmtx;

    h.version = BinRead32Rev(&s->p);
    h.glyphIndex = BinRead32Rev(&s->p);
    h.horizontalBefore = BinRead8(&s->p);
    h.horizontalAfter = BinRead8(&s->p);
    h.horizontalCaretHead = BinRead8(&s->p);
    h.horizontalCaretBase = BinRead8(&s->p);
    h.verticalBefore = BinRead8(&s->p);
    h.verticalAfter = BinRead8(&s->p);
    h.verticalCaretHead = BinRead8(&s->p);
    h.verticalCaretBase = BinRead8(&s->p);

    LOG(
        "\t\tversion: %#x, glyphIndex: %u\n"
        "\t\t\thorizontalBefore: %u, horizontalAfter: %u, horizontalCaretHead: %u, horizontalCaretBase: %u\n"
        "\t\t\tverticalBefore: %u, verticalAfter: %u, verticalCaretHead: %u, verticalCaretBase: %u\n",
        h.version, h.glyphIndex,
        h.horizontalBefore, h.horizontalAfter, h.horizontalCaretHead, h.horizontalCaretBase,
        h.verticalBefore, h.verticalAfter, h.verticalCaretHead, h.verticalCaretBase
    );

    s->p.pos = savedPos;
}

static void
locaRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->locaOffset = at;
    auto& l = s->loca;


    s->p.pos = savedPos;
}

static void
maxpRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->maxpOffset = at;
    auto& m = s->maxp;

    m.version.l = BinRead16Rev(&s->p);
    m.version.r = BinRead16Rev(&s->p);
    m.numGlyphs = BinRead16Rev(&s->p);
    m.maxPoints = BinRead16Rev(&s->p);
    m.maxContours = BinRead16Rev(&s->p);
    m.maxComponentPoints = BinRead16Rev(&s->p);
    m.maxComponentContours = BinRead16Rev(&s->p);
    m.maxZones = BinRead16Rev(&s->p);
    m.maxTwilightPoints = BinRead16Rev(&s->p);
    m.maxStorage = BinRead16Rev(&s->p);
    m.maxFunctionDefs = BinRead16Rev(&s->p);
    m.maxInstructionDefs = BinRead16Rev(&s->p);
    m.maxStackElements = BinRead16Rev(&s->p);
    m.maxSizeofInstruction = BinRead16Rev(&s->p);
    m.maxComponentElements = BinRead16Rev(&s->p);
    m.maxComponentDepth = BinRead16Rev(&s->p);

#ifdef D_TTF
    LOG(
        "\t\tversion: (%d, %d), numGlyphs: %u, maxPoints: %u, maxContours: %u\n"
        "\t\t\tmaxComponentPoints: %u, maxComponentContours: %u, maxZones: %u\n"
        "\t\t\tmaxTwilightPoints: %u, maxStorage: %u, maxFunctionDefs: %u\n"
        "\t\t\tmaxInstructionDefs: %u, maxStackElements: %u, maxSizeofInstruction: %u\n"
        "\t\t\tmaxComponentElements: %u, maxComponentDepth: %u\n",
        m.version.l, m.version.r, m.numGlyphs, m.maxPoints, m.maxContours,
        m.maxComponentPoints, m.maxComponentContours, m.maxZones,
        m.maxTwilightPoints, m.maxStorage, m.maxFunctionDefs,
        m.maxInstructionDefs, m.maxStackElements, m.maxSizeofInstruction,
        m.maxComponentElements, m.maxComponentDepth
    );

#endif

    s->p.pos = savedPos;
}

static void
nameRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->nameOffset = at;
    Name& n = s->name;

    n.format = BinRead16Rev(&s->p);
    n.count = BinRead16Rev(&s->p);
    n.stringOffset = BinRead16Rev(&s->p);

    n.aNameRecords = Vec<NameRecords>(s->p.pAlloc, n.count);
    VecSetSize(&n.aNameRecords, n.count);
    for (auto& nr : n.aNameRecords)
    {
        nr.platformID = BinRead16Rev(&s->p);
        nr.platformSpecificID = BinRead16Rev(&s->p);
        nr.languageID = BinRead16Rev(&s->p);
        nr.nameID = BinRead16Rev(&s->p);
        nr.length = BinRead16Rev(&s->p);
        nr.offset = BinRead16Rev(&s->p);

        String sPlatformID = platformIDToString(nr.platformID);
        String sPlatformSpecificID = platformSpecificIDToString(nr.platformSpecificID);
        String sLanguage = languageIDToString(nr.languageID);

#ifdef D_TTF
        // LOG(
        //     "\t\t(%u): platformID: %u(%.*s), platformSpecificID: %u(%.*s)\n"
        //     "\t\t\tlanguageID: %u(%.*s)\n"
        //     "\t\t\tnameID: %u, length: %u, offset: %u\n",
        //     VecGetIdx(n.aNameRecords, &nr), nr.platformID, sPlatformID.size, sPlatformID.pData,
        //     nr.platformSpecificID, sPlatformSpecificID.size, sPlatformSpecificID.pData,
        //     nr.languageID, sLanguage.size, sLanguage.pData,
        //     nr.nameID, nr.length, nr.offset
        // );
#endif
    }

    LOG("\n\t\t\tformat: %u, count: %u, stringOffset: %u\n",
        n.format, n.count, n.stringOffset
    );

    // TODO: complete this or not bother?

    s->p.pos = savedPos;
}

static void
postRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->postOffset = at;

    s->p.pos = savedPos;
}

static void
FontParse(Font* s)
{
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

    LOG_GOOD(
        "sfntVersion: %u, numTables: %u, searchRange: %u, entrySelector: %u, rangeShift: %u\n",
        td.sfntVersion, td.numTables, td.searchRange, td.entrySelector, td.rangeShift
    );
#endif

    td.aTableRecords = Vec<TableRecord>(s->p.pAlloc, td.numTables); 
    VecSetSize(&td.aTableRecords, td.numTables);
    for (auto& r : s->tableDirectory.aTableRecords)
    {
        r.tableTag = BinReadString(&s->p, 4);
        r.checkSum = BinRead32Rev(&s->p);
        r.offset = BinRead32Rev(&s->p);
        r.length = BinRead32Rev(&s->p);

#ifdef D_TTF
        LOG(
            "(%u): tableTag: '%.*s'(%u), checkSum: %u, offset: %u, length: %u\n",
            VecGetIdx(&s->tableDirectory.aTableRecords, &r),
            r.tableTag.size, r.tableTag.pData, *(u32*)(r.tableTag.pData), r.checkSum, r.offset, r.length
        );
#endif

        /* get required tables */
        if (r.tableTag == "cmap")
            cmapRead(s, r.offset);
        else if (r.tableTag == "glyf")
            glyfRead(s, r.offset);
        else if (r.tableTag == "head")
            headRead(s, r.offset);
        else if (r.tableTag == "hhea")
            hheaRead(s, r.offset);
        else if (r.tableTag == "hmtx")
            hmtxRead(s, r.offset);
        else if (r.tableTag == "loca")
            locaRead(s, r.offset);
        else if (r.tableTag == "maxp")
            maxpRead(s, r.offset);
        else if (r.tableTag == "name")
            nameRead(s, r.offset);
        else if (r.tableTag == "post")
            postRead(s, r.offset);
    }
}

void
FontDestroy(Font* s)
{
    // TODO:
}

u32
FontGetGlyphOffset(Font* s, u32 idx)
{
    const auto f = s->p.sFile;
    u32 offset = 0, old = 0;

    if (s->head.indexToLocFormat == 1)
    {
    }

    return 0;
}

} /* namespace ttf */
} /* namespace parser */
