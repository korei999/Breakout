#include "Lexer.hh"

#include <cctype>

using namespace adt;

namespace json
{

Token
Lexer::next()
{
    skipWhitespace();
    if (m_pos >= m_sJson.getSize()) return {};

    Token tok {};

    switch (m_sJson[m_pos])
    {
        case '{':
        tok = nextChar(TOKEN_TYPE::L_BRACE);
        advanceOne();
        break;

        case '}':
        tok = nextChar(TOKEN_TYPE::R_BRACE);
        advanceOne();
        break;

        case '"':
        tok = nextString();
        advanceOne();
        break;

        case ':':
        tok = nextChar(TOKEN_TYPE::COLON);
        advanceOne();
        break;

        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '+':
        tok = nextNumber();
        break;

        case '[':
        tok = nextChar(TOKEN_TYPE::L_BRACKET);
        advanceOne();
        break;

        case ']':
        tok = nextChar(TOKEN_TYPE::R_BRACKET);
        advanceOne();
        break;

        case ',':
        tok = nextChar(TOKEN_TYPE::COMMA);
        advanceOne();
        break;

        default:
        tok = nextStringNoQuotes();
        break;
    }

    return tok;
}

void
Lexer::skipWhitespace()
{
    auto oneOf = [](char ch) -> bool {
        constexpr char nts[] = " \n\r\t\v\f";
        for (auto& c : nts)
            if (ch == c) return true;

        return false;
    };

    while (m_pos < m_sJson.getSize() && oneOf(m_sJson[m_pos]))
    {
        if (m_sJson[m_pos] == '\n')
        {
            ++m_row;
            m_column = 0;
        }

        advanceOne();
    }
}

Token
Lexer::nextChar(TOKEN_TYPE eType)
{
    return {
        .eType = eType,
        .sLiteral = {&m_sJson[m_pos], 1},
        .row = static_cast<decltype(Token::row)>(m_row),
        .column = static_cast<decltype(Token::column)>(m_column),
    };
}


Token
Lexer::nextString()
{
    ADT_ASSERT(m_sJson[m_pos] == '"', " ");

    advanceOne();
    if (done()) return {};

    auto fPos = m_pos;

    bool bEsc = false;
    for (; m_pos < m_sJson.getSize(); advanceOne())
    {
        if (!bEsc && m_sJson[m_pos] == '\\')
        {
            bEsc = true;
            continue;
        }

        if (m_sJson[m_pos] == '"' && !bEsc)
            break;

        bEsc = false;
    }

    return {
        .eType = TOKEN_TYPE::QUOTED_STRING,
        .sLiteral = {&m_sJson[fPos], m_pos - fPos},
        .row = static_cast<decltype(Token::row)>(m_row),
        .column = static_cast<decltype(Token::column)>(m_column - (m_pos - fPos + 1)),
    };
}

Token
Lexer::nextStringNoQuotes()
{
    auto fPos = m_pos;

    while (m_pos < m_sJson.getSize() && isalnum(m_sJson[m_pos]))
        advanceOne();

    return {
        .eType = TOKEN_TYPE::STRING,
        .sLiteral = {&m_sJson[fPos], m_pos - fPos},
        .row = static_cast<decltype(Token::row)>(m_row),
        .column = static_cast<decltype(Token::column)>(m_column - (m_pos - fPos)),
    };
}

Token
Lexer::nextNumber()
{
    ADT_ASSERT(std::isdigit(m_sJson[m_pos]), " ");

    auto fPos = m_pos;
    TOKEN_TYPE eType = TOKEN_TYPE::NUMBER;

    while (
        m_pos < m_sJson.getSize() &&
        (isdigit(m_sJson[m_pos])  ||
            m_sJson[m_pos] == '.' ||
            m_sJson[m_pos] == '+' ||
            m_sJson[m_pos] == '-'
        )
    )
    {
        if (m_sJson[m_pos] == '.') 
            eType = TOKEN_TYPE::FLOAT;

        advanceOne();
    }

    return {
        .eType = eType,
        .sLiteral = {&m_sJson[fPos], m_pos - fPos},
        .row = static_cast<decltype(Token::row)>(m_row),
        .column = static_cast<decltype(Token::column)>(m_column - (m_pos - fPos)),
    };
}

} /* namespace json */
