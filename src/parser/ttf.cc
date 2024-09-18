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

    for (auto& st : cmap.aSubtables)
    {
        st.platformID = BinRead16Rev(&s->p);
        st.platformSpecificID = BinRead16Rev(&s->p);
        st.offset = BinRead32Rev(&s->p);

        String sPlatformID = platformIDToString(st.platformID);
        String sPlatformSpecificID = platformSpecificIDToString(st.platformSpecificID);

        LOG(
            "\t\t(%u): platformID: %u(%.*s), platformSpecificID: %u(%.*s), offset(%u)\n",
            VecGetIdx(cmap.aSubtables, &st),
            st.platformID, sPlatformID.size, sPlatformID.pData,
            st.platformSpecificID, sPlatformSpecificID.size, sPlatformSpecificID.pData,
            st.offset
        );
    }

    s->p.pos = savedPos;
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

    // TODO: complete

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

    s->p.pos = savedPos;
}

static void
maxpRead(Font* s, u32 at)
{
    u32 savedPos = s->p.pos;
    s->p.pos = at;

    s->maxpOffset = at;

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
        LOG(
            "\t\t(%u): platformID: %u(%.*s), platformSpecificID: %u(%.*s)\n"
            "\t\t\tlanguageID: %u(%.*s)\n"
            "\t\t\tnameID: %u, length: %u, offset: %u\n",
            VecGetIdx(n.aNameRecords, &nr), nr.platformID, sPlatformID.size, sPlatformID.pData,
            nr.platformSpecificID, sPlatformSpecificID.size, sPlatformSpecificID.pData,
            nr.languageID, sLanguage.size, sLanguage.pData,
            nr.nameID, nr.length, nr.offset
        );
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

    u32 sfntVersion = BinRead32Rev(&s->p);
    u16 numTables = BinRead16Rev(&s->p);
    u16 searchRange = BinRead16Rev(&s->p);
    u16 entrySelector = BinRead16Rev(&s->p);
    u16 rangeShift = BinRead16Rev(&s->p);

    if (sfntVersion != 0x00010000 && sfntVersion != 0x4f54544f)
        LOG_FATAL("Unable to read this ('%.*s') ttf header: sfntVersion: %u'\n", s->p.sPath.size, s->p.sPath.pData, sfntVersion);

#ifdef D_TTF
    u16 _searchRangeCheck = pow(2, floor(log2(numTables))) * 16;
    assert(searchRange == _searchRangeCheck);

    u16 _entrySelectorCheck = (u16)floor(log2(numTables));
    assert(entrySelector == _entrySelectorCheck);

    u16 _rangeShiftCheck = numTables*16 - searchRange;
    assert(rangeShift == _rangeShiftCheck);

    LOG_GOOD(
        "sfntVersion: %u, numTables: %u, searchRange: %u, entrySelector: %u, rangeShift: %u\n",
        sfntVersion, numTables, searchRange, entrySelector, rangeShift
    );
#endif

    s->aTableDirectories = Vec<TableDirectory>(s->p.pAlloc, numTables); 
    VecSetSize(&s->aTableDirectories, numTables);

    for (u32 i = 0; i < numTables; i++)
    {
        auto& t = s->aTableDirectories[i];

        t.tableTag = BinReadString(&s->p, 4);
        t.checkSum = BinRead32Rev(&s->p);
        t.offset = BinRead32Rev(&s->p);
        t.length = BinRead32Rev(&s->p);

#ifdef D_TTF
        LOG(
            "(%u): tableTag: '%.*s'(%u), checkSum: %u, offset: %u, length: %u\n",
            i, t.tableTag.size, t.tableTag.pData, *(u32*)(t.tableTag.pData), t.checkSum, t.offset, t.length
        );
#endif

        /* get required tables */
        if (t.tableTag == "cmap")
            cmapRead(s, t.offset);
        else if (t.tableTag == "glyf")
            glyfRead(s, t.offset);
        else if (t.tableTag == "head")
            headRead(s, t.offset);
        else if (t.tableTag == "hhea")
            hheaRead(s, t.offset);
        else if (t.tableTag == "hmtx")
            hmtxRead(s, t.offset);
        else if (t.tableTag == "loca")
            locaRead(s, t.offset);
        else if (t.tableTag == "maxp")
            maxpRead(s, t.offset);
        else if (t.tableTag == "name")
            nameRead(s, t.offset);
        else if (t.tableTag == "post")
            postRead(s, t.offset);
    }
}

void
FontDestroy(Font* s)
{
    // TODO:
}

} /* namespace ttf */
} /* namespace parser */
