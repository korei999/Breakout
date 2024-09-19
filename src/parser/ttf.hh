#pragma once

#include "Bin.hh"
#include "adt/String.hh"
#include "adt/Vec.hh"

namespace parser
{
namespace ttf
{

struct Font;

bool FontLoad(Font* s, String path);
void FontDestroy(Font* s);

/* https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html */
/* https://learn.microsoft.com/en-us/typography/opentype/spec/otff */
/* https://handmade.network/forums/articles/t/7330-implementing_a_font_reader_and_rasterizer_from_scratch%252C_part_1__ttf_font_reader. */

using Fixed = struct { s16 l; s16 r; }; /* 32-bit signed fixed-point number (16.16) */
using fixed32 = u32; /* TODO: wtf is this type? */
using FWord = s16; /* int16 that describes a quantity in font design units. */
using uFWord = u16; /* uint16 that describes a quantity in font design units. */
using F2Dot14 = s16; /* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
using longDateTime = s64; /* Date and time represented in number of seconds
                           * since 12:00 midnight, January 1, 1904, UTC.
                           * The value is represented as a signed 64-bit integer. */
using Tag = u8[4]; /* Array of four uint8s (length = 32 bits) used to identify a table,
                    * design-variation axis, script, language system, feature, or baseline. */
using Offset8 = u8; /* 8-bit offset to a table, same as uint8, NULL offset = 0x00 */
using Offset16 = u16; /* Short offset to a table, same as uint16, NULL offset = 0x0000 */
using Offset24 = u24; /* 24-bit offset to a table, same as uint24, NULL offset = 0x000000 */
using Offset32 = u32; /* Long offset to a table, same as uint32, NULL offset = 0x00000000 */
using Version16Dot16 = struct { u16 maj; u16 min; }; /*Packed 32-bit value with major and minor version numbers. */

struct TableRecord
{
    String tableTag {}; /* instead of TTF_Tag */
    u32 checkSum {};
    Offset32 offset {};
    u32 length {};
};

struct TableDirectory
{
    u32 sfntVersion; /* 0x00010000 or 0x4F54544F ('OTTO') */
    u16 numTables; /* Number of tables. */
    u16 searchRange; /* Maximum power of 2 less than or equal to numTables,
                      * times 16 ((2**floor(log2(numTables))) * 16, where “**” is an exponentiation operator). */
    u16 entrySelector; /* Log2 of the maximum power of 2 less than or equal to numTables (log2(searchRange/16),
                        * which is equal to floor(log2(numTables))). */
    u16 rangeShift; /* numTables times 16, minus searchRange ((numTables * 16) - searchRange). */
    Vec<TableRecord> aTableRecords;
};

struct Kern
{
};

struct CMAPEncodingSubtable
{
    u16 platformID; /* Platform identifier:
                     * 0 Unicode Indicates Unicode version.
                     * 1 Macintosh Script Manager code.
                     * 2 (reserved; do not use).
                     * 3 Microsoft encoding. */
    u16 platformSpecificID; /* Platform-specific identifier:
                             * 0 Version 1.0 semantics.
                             * 1 Version 1.1 semantics.
                             * 2 ISO 10646 1993 semantics (deprecated).
                             * 3 Unicode 2.0 or later semantics (BMP only).
                             * 4 Unicode 2.0 or later semantics (non-BMP characters allowed).
                             * 5 Unicode Variation Sequences.
                             * 6 Last Resort. */
    u32 offset; /* Offset of the mapping table */
};

struct Cmap
{
    u16 version; /* Version number (Set to zero) */
    u16 numberSubtables; /* Number of encoding subtables */
    Vec<CMAPEncodingSubtable> aSubtables;
};

enum OUTLINE_FLAGS : u8
{
    ON_CURVE = 0, /* If set, the point is on the curve;
                   * Otherwise, it is off the curve. */
    X_SHORT_VECTOR, /* If set, the corresponding x-coordinate is 1 byte long;
                     * Otherwise, the corresponding x-coordinate is 2 bytes long */
    Y_SHORT_VECTOR, /* If set, the corresponding y-coordinate is 1 byte long;
                     * Otherwise, the corresponding y-coordinate is 2 bytes long */
    REPEAT, /* If set, the next byte specifies the number of additional times this set of flags is to be repeated.
             * In this way, the number of flags listed can be smaller than the number of points in a character. */
    THIS_X_IS_SAME, /* This flag has one of two meanings, depending on how the x-Short Vector flag is set.
                     * If the x-Short Vector bit is set, this bit describes the sign of the value, with a value of 1 equalling positive and a zero value negative.
                     * If the x-short Vector bit is not set, and this bit is set, then the current x-coordinate is the same as the previous x-coordinate.
                     * If the x-short Vector bit is not set, and this bit is not set, the current x-coordinate is a signed 16-bit delta vector. In this case, the delta vector is the change in x */
    THIS_Y_IS_SAME, /* This flag has one of two meanings, depending on how the y-Short Vector flag is set.
                     * If the y-Short Vector bit is set, this bit describes the sign of the value, with a value of 1 equalling positive and a zero value negative.
                     * If the y-short Vector bit is not set, and this bit is set, then the current y-coordinate is the same as the previous y-coordinate.
                     * If the y-short Vector bit is not set, and this bit is not set, the current y-coordinate is a signed 16-bit delta vector. In this case, the delta vector is the change in y */
    RESERVED6, /* set to zero */
    RESERVED7, /* set to zero */
};

struct GlyphSimple
{
    Vec<u16> aEndPtsOfContours; /* [n] Array of last points of each contour; n is the number of contours; array entries are point indices */
    u16 instructionLength; /* Total number of bytes needed for instructions */
    Vec<u8> aInstructions; /* [instructionLength] Array of instructions for this glyph */
    Vec<OUTLINE_FLAGS> aeFlags; /* [variable] Array of flags */
    void* pXCoordinates; /* (u8 or s16) Array of x-coordinates; the first is relative to (0,0), others are relative to previous point */
    void* pYCoordinates; /* (u8 or s16) Array of y-coordinates; the first is relative to (0,0), others are relative to previous point */
};

enum COMPONENT_FLAG : u16
{
    ARG_1_AND_2_ARE_WORDS = 0, /* If set, the arguments are words;
                                * If not set, they are bytes. */
    ARGS_ARE_XY_VALUES, /* If set, the arguments are xy values;
                         * If not set, they are points. */
    ROUND_XY_TO_GRID, /* If set, round the xy values to grid;
                       * if not set do not round xy values to grid (relevant only to bit 1 is set) */
    WE_HAVE_A_SCALE, /* If set, there is a simple scale for the component.
                      * If not set, scale is 1.0. */
    __THIS_BIT_IS_OBSOLETE, /* (obsolete; set to zero) */
    MORE_COMPONENST, /* If set, at least one additional glyph follows this one. */
    WE_HAVE_AN_X_AND_Y_SCALE, /* If set the x direction will use a different scale than the y direction. */
    WE_HAVE_A_TWO_BY_TWO, /* If set there is a 2-by-2 transformation that will be used to scale the component. */
    WE_HAVE_INSTRUCTIONS, /* If set, instructions for the component character follow the last component. */
    USE_MY_METRICS, /* Use metrics from this component for the compound glyph. */
    OVERLAP_COMPOUND, /* If set, the components of this compound glyph overlap. */
};

/* x' = m((a/m)*x + (c/m)*y + e)
 * y' = n((b/n)*x + (d/n)*y + f) */

/* The values of a, b, c, and d are obtained as shown in Table 19a. The values of e and f are obtained as shown in the following pseudo-code below:
 * 
 * if (ARG_1AND_2_ARE_WORDS && ARGS_ARE_XY_VALUES)
 *     1st short contains the value of e
 *     2nd short contains the value of f
 * else if (!ARG_1AND_2_ARE_WORDS && ARGS_ARE_XY_VALUES)
 *     1st byte contains the value of e
 *     2nd byte contains the value of f
 * else if (ARG_1AND_2_ARE_WORDS && !ARGS_ARE_XY_VALUES)
 *     1st short contains the index of matching point in compound being constructed
 *     2nd short contains index of matching point in component
 * else if (!ARG_1AND_2_ARE_WORDS && !ARGS_ARE_XY_VALUES)
 *     1st byte containing index of matching point in compound being constructed
 *     2nd byte containing index of matching point in component */

/* Finally, m and n are calculated as follows:
 * First, let m₀ = max(|a|, |b|) and n₀ = max(|c|, |d|).
 * If |(|a|-|c|)| ≤ 33/65536, then m = 2m₀; otherwise, m = m₀.
 * Similarly, if |(|b|-|d|)| ≤ 33/65536, then n = 2n₀; otherwise, n = n₀
 * The linear transformation data is derived as shown in Table 19a. */

/* Table 19a Linear transformation:
 * WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | a         | b         | c         | d
 * ----------------+--------------------------+-----------+-----------+-----------+-----------
 * 0               | 0                        | 1.0       | 0.0       | 0.0       | 1.0       
 * 1               | 0                        | 1st short | 0.0       | 0.0       | 1st short 
 * 0               | 1                        | 1st short | 0.0       | 0.0       | 2nd short 
 * 0               | 0                        | 1st short | 2nd short | 3rd short | 4th short  */

/* The numbers stored as shorts are treated as signed fixed binary point numbers with one bit to the left of the binary point and 14 to the right.
 * 
 * The application of the transformations and instructions can be thought of as occurring in the following order:
 * 
 * For each component
 * 
 * Apply the local transformation, if any
 * Apply the global transformation (e.g. point size, resolution)
 * Gridfit the outline
 * Apply the translation from offsets or anchor points
 * Gridfit the compound glyph if there are instructions */

/* Important:
 *
 * If e and f are specified directly as offsets rather than as point indices to be matched,
 * then the correct values of the offsets depend on the transformation components.For example,
 * if a general scale factor of one - half is applied to the component,
 * this scale factor is also applied to the values of e and f before they are used to offset the component.Note also,
 * that when the component is rotated by a multiple of 45 degrees, the scale factors are doubled.For these reasons,
 * it is much easier to specify offsets through the use of anchor point and matching points than directly through offset values. */

// TODO: not sure about this struct
union TransformationOptions
{
    struct {
        F2Dot14 scale; /* (same for x and y) */
    } entry1;

    struct {
        F2Dot14 xScale;
        F2Dot14 yScale;
    } entry2;

    struct {
        F2Dot14 xScale;
        F2Dot14 scale01;
        F2Dot14 scale10;
        F2Dot14 yScale;
    } entry3;
};

struct GlyphCompound
{
    COMPONENT_FLAG eFlag; /* Component flag */
    u16 glyphIndex; /* Glyph index of component */
    void* pArgument1; /* (s16, u16, s8 or u8) X-offset for component or point number; type depends on bits 0 and 1 in component flags */
    void* pArgument2; /* (s16, u16, s8 or u8) Y-offset for component or point number; type depends on bits 0 and 1 in component flags */
    TransformationOptions uTransformationOptions; /* One of the transformation options from Table 19 */
};

struct Glyph
{
    s16 numberOfContours; /* If the number of contours is positive or zero, it is a single glyph;
                           * If the number of contours less than zero, the glyph is compound */
    FWord xMin; /* Minimum x for coordinate data */
    FWord yMin; /* Minimum y for coordinate data */
    FWord xMax; /* Maximum x for coordinate data */
    FWord yMax; /* Maximum y for coordinate data */
    union {
        GlyphSimple simple;
        GlyphCompound compound;
    } uGlyph;
};

/* Many of the fields in the 'head' table are closely related to the values in other tables.
 * For example, the unitsPerEm field is fundamental to all tables which deal with curves or metrics.
 * This table is used throughout the TrueType rendering engine as a short-cut to determine various key aspects of the font.
 * Inasmuch as any change to any other table will have an impact on the checkSumAdjustment and modified fields of the 'head' table,
 * it is always wisest to update the 'head' table after any font editing operation. */
struct Head
{
    Fixed version; /* 0x00010000 if (version 1.0) */
    Fixed fontRevision; /* set by font manufacturer */
    u32 checkSumAdjustment; /* To compute: set it to 0,
                             * calculate the checksum for the 'head' table and put it in the table directory,
                             * sum the entire font as a uint32_t, then store 0xB1B0AFBA - sum.
                             * (The checksum for the 'head' table will be wrong as a result.
                             * That is OK; do not reset it.) */
    u32 magicNumber; /* set to 0x5F0F3CF5 */
    u16 flags; /* bit 0 - y value of 0 specifies baseline.
                * bit 1 - x position of left most black bit is LSB.
                * bit 2 - scaled point size and actual point size will differ (i.e. 24 point glyph differs from 12 point glyph scaled by factor of 2).
                * bit 3 - use integer scaling instead of fractional.
                * bit 4 - (used by the Microsoft implementation of the TrueType scaler).
                * bit 5 - This bit should be set in fonts that are intended to e laid out vertically, and in which the glyphs have been drawn such that an x-coordinate of 0 corresponds to the desired vertical baseline.
                * bit 6 - This bit must be set to zero.
                * bit 7 - This bit should be set if the font requires layout for correct linguistic rendering (e.g. Arabic fonts).
                * bit 8 - This bit should be set for an AAT font which has one or more metamorphosis effects designated as happening by default.
                * bit 9 - This bit should be set if the font contains any strong right-to-left glyphs.
                * bit 10 - This bit should be set if the font contains Indic-style rearrangement effects.
                * bits 11-13 - Defined by Adobe.
                * bit 14 - This bit should be set if the glyphs in the font are simply generic symbols for code point ranges, such as for a last resort font. */
    u16 unitsPerEm; /* range from 64 to 16384 */
    longDateTime created; /* international date */
    longDateTime modified; /* international date */
    FWord xMin; /* for all glyph bounding boxes */
    FWord yMin; /* for all glyph bounding boxes */
    FWord xMax; /* for all glyph bounding boxes */
    FWord yMax; /* for all glyph bounding boxes */
    u16 macStyle; /* bit 0 bold.
                   * bit 1 italic.
                   * bit 2 underline.
                   * bit 3 outline.
                   * bit 4 shadow.
                   * bit 5 condensed (narrow).
                   * bit 6 extended. */
    u16 lowestRecPPEM; /* smallest readable size in pixels */
    s16 fontDirectionHint; /* 0 Mixed directional glyphs
                            * 1 Only strongly left to right glyphs
                            * 2 Like 1 but also contains neutrals
                            * -1 Only strongly right to left glyphs
                            * -2 Like -1 but also contains neutrals */
    s16 indexToLocFormat; /* 0 for short offsets, 1 for long */
    s16 glyphDataFormat; /* 0 for current format */
};

/* Dependencies:
 * Other tables may have information duplicating data contained in the 'hhea' table,
 * most notably the ascent and descent fields.
 * Such information may be found in tables such as the 'OS/2' table or the 'bloc' table.
 * Care should always be taken that metric information within a font is consistent,
 * as different applications and systems get the metric information from different places.
 * This is particularly true for fonts intended for cross-platform use. E.g.,
 * Windows uses the 'OS/2' table as the basic source for ascent and descent for the font.
 * The caret slope calculated as the ratio between caretSlopeRise and caretSlopeRun should be equal to tan(90° + p),
 * where p is the value of the italicAngle field in the 'post' table.
 * The value of the numOfLongHorMetrics field is used by the 'hmtx' (Horizontal Metrics) table.
 * Fonts that lack an 'hhea' table must not have an 'hmtx' table. */
struct Hhea
{
    Fixed version; /* 0x00010000 (1.0) */
    FWord ascent; /* Distance from baseline of highest ascender */
    FWord descent; /* Distance from baseline of lowest descender */
    FWord lineGap; /* typographic line gap */
    uFWord advanceWidthMax; /* must be consistent with horizontal metrics */
    FWord minLeftSideBearing; /* must be consistent with horizontal metrics */
    FWord minRightSideBearing; /* must be consistent with horizontal metrics */
    FWord xMaxExtent; /* max(lsb + (xMax-xMin)) */
    s16 caretSlopeRise; /* used to calculate the slope of the caret (rise/run) set to 1 for vertical caret */
    s16 caretSlopeRun; /* 0 for vertical */
    FWord caretOffset; /* set value to 0 for non-slanted fonts */
    s16 reserved0; /* set value to 0 */
    s16 reserved1; /* set value to 0 */
    s16 reserved2; /* set value to 0 */
    s16 reserved3; /* set value to 0 */
    s16 metricDataFormat; /* 0 for current format */
    u16 numOfLongHorMetrics; /* number of advance widths in metrics table */
};

/* The Font Metrics Table(tag : 'fmtx')
 * identifies a glyph whose points represent various font-wide metrics: ascent, descent, caret angle, caret offset.
 * If this table is present, these points override their corresponding values in the 'hhea' and 'vhea' tables.
 * Representing these metrics as points rather that values offers several advantages.
 *
 * In Quickdraw Text, glyphs are constrained to not exceed the font's ascent and descent.
 * Because of this, many fonts artificially increase their ascent and descent values in the 'hhea' table to accomodate exceptionally tall or low glyphs.
 * The 'fmtx' is only used in ATSUI, allowing the same font to specify "real" values for ascent and descent without changing the behavior in Quickdraw Text.
 * Since 'fmtx' metrics are represented as points, they can change for different variation settings,
 * allowing the font to change its line spacing and/or caret angle as it changes weight, or optical point size.
 * 'fmtx' metric points can also be instructed, allowing the font to tune its line spacing at small sizes for improved readability. */
struct Hmtx
{
    fixed32 version; /* Version (set to 0x00020000). */
    u32 glyphIndex; /* The glyph whose points represent the metrics. */
    u8 horizontalBefore; /* Point number for the horizontal ascent. */
    u8 horizontalAfter; /* Point number for the horizontal descent. */
    u8 horizontalCaretHead; /* Point number for the horizontal caret head. */
    u8 horizontalCaretBase; /* Point number for the horizontal caret base. */
    u8 verticalBefore; /* Point number for the vertical ascent. */
    u8 verticalAfter; /* Point number for the vertical descent. */
    u8 verticalCaretHead; /* Point number for the vertical caret head. */
    u8 verticalCaretBase; /* Point number for the vertical caret base. */
};

/* https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6loca.html
 * General table information;
 * The 'loca' table stores the offsets to the locations of the glyphs in the font relative to the beginning of the 'glyf' table.
 * Its purpose is to provide quick access to the data for a particular character.
 * For example, in the standard Macintosh glyph ordering, the character A is the 76th glyph in a font.
 * The 'loca' table stores the offset from the start of the 'glyf' table to the position at which the data for each glyph can be found.
 * There are two versions of this table, the short and the long.
 * The version used is specified in the Font Header ('head') table in the indexToLocFormat field.
 * The choice of long or short offsets is dependent on the maximum possible offset distance.
 * This table is an array of n offsets where n is the number of glyphs in the font plus one (for the extra entry mentioned above).
 * The actual offset for any particular glyph is divided by 2.
 * The number of glyphs in a font must be stored in the Maximum Profile 'maxp' table. */
struct Loca
{
    union {
        Vec<u16> shortVersion;
        Vec<u32> longVersion;
    } offsets;
};

struct Maxp
{
    Fixed version; /* 0x00010000 (1.0) */
    u16 numGlyphs; /* the number of glyphs in the font */
    u16 maxPoints; /* points in non-compound glyph */
    u16 maxContours; /* contours in non-compound glyph */
    u16 maxComponentPoints; /* points in compound glyph */
    u16 maxComponentContours; /* contours in compound glyph*/
    u16 maxZones; /* set to 2 */
    u16 maxTwilightPoints; /* points used in Twilight Zone (Z0) */
    u16 maxStorage; /* number of Storage Area locations */
    u16 maxFunctionDefs; /* number of FDEFs */
    u16 maxInstructionDefs; /* number of IDEFs */
    u16 maxStackElements; /* maximum stack depth */
    u16 maxSizeofInstruction; /* byte count for glyph instructions */
    u16 maxComponentElements; /* number of glyphs referenced at top level */
    u16 maxComponentDepth; /* levels of recursion, set to 0 if font has only simple glyphs */
};

/* https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6name.html */
struct NameRecords
{
    u16 platformID; /* For names with a Unicode platformID, the language code is unused and should be set to 0. */
    u16 platformSpecificID;
    u16 languageID;
    u16 nameID;
    u16 length; /* Name string length in bytes. */
    u16 offset; /* Name string offset in bytes from stringOffset. */
};

/* Dependencies:
 * The 'name' table itself does not depend on other tables in the font.
 * However, other tables (such as the 'feat' (feature name)
 * Table or the 'fvar' (font variations) table) contain name identifiers
 * Which are to be used for UI elements providing control of font features or variations.
 * Any name identifier found in one of these other tables must have at least one instance in the 'name' table. */
struct Name
{
    u16 format; /* Format selector. Set to 0. */
    u16 count; /* The number of nameRecords in this name table. */
    u16 stringOffset; /* Offset in bytes to the beginning of the name character strings. */
    Vec<NameRecords> aNameRecords; /* [count] The name records array. */
    String name; /* Character strings. The character strings of the names. Note that these are not necessarily ASCII! */
};

struct Post
{
};

struct Font
{
    Bin p {};
    TableDirectory tableDirectory;
    Cmap cmap {};
    Glyph glyph {};
    Kern kern {};
    Head head {};
    Hhea hhea {};
    Hmtx hmtx {};
    Loca loca {};
    Maxp maxp {};
    Name name {};
    Post post {};
    u32 cmapOffset {};
    u32 glyphOffset {};
    u32 headOffset {};
    u32 hheaOffset {};
    u32 hmtxOffset {};
    u32 locaOffset {};
    u32 maxpOffset {};
    u32 nameOffset {};
    u32 postOffset {};

    Font() = default;
    Font(Allocator* p) : p {p} {}
};

} /* namespace ttf */
} /* namespace parser */
