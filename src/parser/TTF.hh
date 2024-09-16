#pragma once

#include "Bin.hh"
#include "adt/String.hh"

using namespace adt;

namespace parser
{

/* https://learn.microsoft.com/en-us/typography/opentype/spec/otff */

using TTF_Fixed = struct { s16 i; s16 f; }; /* 32-bit signed fixed-point number (16.16) */
using TTF_FWORD = s16; /* int16 that describes a quantity in font design units. */
using TTF_UFWORD = u16; /* uint16 that describes a quantity in font design units. */
using TTF_F2DOT14 = s16; /* 16-bit signed fixed number with the low 14 bits of fraction (2.14). */
using TTF_LONGDATETIME = s64; /* Date and time represented in number of seconds
                           * since 12:00 midnight, January 1, 1904, UTC.
                           * The value is represented as a signed 64-bit integer. */
using TTF_Tag = u8[4]; /* Array of four uint8s (length = 32 bits) used to identify a table,
                    * design-variation axis, script, language system, feature, or baseline. */
using TTF_Offset8 = u8; /* 8-bit offset to a table, same as uint8, NULL offset = 0x00 */
using TTF_Offset16 = u16; /* Short offset to a table, same as uint16, NULL offset = 0x0000 */
using TTF_Offset24 = u24; /* 24-bit offset to a table, same as uint24, NULL offset = 0x000000 */
using TTF_Offset32 = u32; /* Long offset to a table, same as uint32, NULL offset = 0x00000000 */
using TTF_Version16Dot16 = struct { u16 maj; u16 min; }; /*Packed 32-bit value with major and minor version numbers. */

struct TableRecord
{
    TTF_Tag tableTag {};
    u32 checkSum {};
    TTF_Offset32 offset {};
    u32 length {};
};

struct TTF
{
    Bin p {};

    TTF() = default;
    TTF(Allocator* p) : p{p} {}

};

void TTFLoad(TTF* s, String path);
void TTFDestroy(TTF* s);

} /* namespace parser */
