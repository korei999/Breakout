#pragma once

#include "adt/print.hh"
#include "adt/enum.hh"

namespace json
{

enum class TOKEN_TYPE : adt::u32
{
    NONE = 0,
    DOT = 1,
    COMMA = 1 << 1,
    STRING = 1 << 2,
    QUOTED_STRING = 1 << 3,
    COLON = 1 << 4,
    L_BRACE = 1 << 5,
    R_BRACE = 1 << 6,
    L_BRACKET = 1 << 7,
    R_BRACKET = 1 << 8,
    NUMBER = 1 << 9,
    FLOAT = 1 << 10,
};
ADT_ENUM_BITWISE_OPERATORS(TOKEN_TYPE);

struct Token
{
    TOKEN_TYPE eType {};
    adt::String sLiteral {};
    adt::u32 row {};
    adt::u32 column {};
};

class Lexer
{
    adt::String m_sJson {};
    adt::u32 m_pos {};
    adt::u32 m_row = 1;
    adt::u32 m_column = 1;

    /* */

public:
    Lexer() = default;
    Lexer(adt::String sJson) : m_sJson(sJson), m_row(1), m_column(1) {}

    /* */

    Token next();
    bool done() const { return m_pos >= m_sJson.getSize(); }

    /* */

private:
    void skipWhitespace();
    Token nextChar(TOKEN_TYPE eType);
    Token nextString();
    Token nextStringNoQuotes();
    Token nextNumber();

    void advanceOne() { ++m_pos, ++m_column; }
};

} /* namespace json */

namespace adt
{
namespace print
{

inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const json::TOKEN_TYPE& x)
{
    ctx.fmt = "{}";
    ctx.fmtIdx = 0;

    char aBuff[256] {};
    u32 n = 0;

    if (u32(x & json::TOKEN_TYPE::NONE))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, "NONE");
    if (u32(x & json::TOKEN_TYPE::DOT))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | DOT" : "DOT");
    if (u32(x & json::TOKEN_TYPE::COMMA))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | COMMA" : "COMMA");
    if (u32(x & json::TOKEN_TYPE::STRING))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | STRING" : "STRING");
    if (u32(x & json::TOKEN_TYPE::QUOTED_STRING))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | QUOTED_STRING" : "QUOTED_STRING");
    if (u32(x & json::TOKEN_TYPE::COLON))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | COLON" : "COLON");
    if (u32(x & json::TOKEN_TYPE::L_BRACE))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | L_BRACE" : "L_BRACE");
    if (u32(x & json::TOKEN_TYPE::R_BRACE))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | R_BRACE" : "R_BRACE");
    if (u32(x & json::TOKEN_TYPE::L_BRACKET))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | L_BRACKET" : "L_BRACKET");
    if (u32(x & json::TOKEN_TYPE::R_BRACKET))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | R_BRACKET" : "R_BRACKET");
    if (u32(x & json::TOKEN_TYPE::NUMBER))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | NUMBER" : "NUMBER");
    if (u32(x & json::TOKEN_TYPE::FLOAT))
        n += print::toBuffer(aBuff + n, sizeof(aBuff) - n - 1, n > 0 ? " | FLOAT" : "FLOAT");

    return printArgs(ctx, aBuff);
}

} /* namespace print */
} /* namespace adt */
