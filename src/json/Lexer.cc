#include "Lexer.hh"

#include "adt/file.hh"
#include "adt/logs.hh"

#include <ctype.h>

using namespace adt;

namespace json
{

static void skipWhiteSpace(Lexer* s);
static Token number(Lexer* s);
static Token stringNoQuotes(Lexer* s);
static Token string(Lexer* s);
static Token character(Lexer* s, Token::TYPE type);

void
Lexer::destroy(adt::IAllocator* pAlloc)
{
    pAlloc->free(m_sFile.m_pData);
}

RESULT Lexer::loadFile(IAllocator* pAlloc, adt::String path)
{
    Opt<String> rs = file::load(pAlloc, path);
    if (!rs) return FAIL;

    m_sFile = rs.value();
    m_pos = 0;

    return OK;
}

static void
skipWhiteSpace(Lexer* s)
{
    auto oneOf = [](char c) -> bool {
        const char skipChars[] = " \t\n\r";

        for (auto ch : skipChars)
            if (c == ch) return true;

        return false;
    };

    while (s->m_pos < s->m_sFile.m_size && oneOf(s->m_sFile[s->m_pos]))
        s->m_pos++;
}

static Token
number(Lexer* s)
{
    Token r {};
    u32 start = s->m_pos;
    u32 i = start;

    while (
        isxdigit(s->m_sFile[i]) ||
        s->m_sFile[i] == '.'    ||
        s->m_sFile[i] == '-'    ||
        s->m_sFile[i] == '+'
    )
    {
        i++;
    }

    r.type = Token::NUMBER;
    r.sLiteral = {&s->m_sFile[start], i - start};
    
    s->m_pos = i - 1;
    return r;
}

static Token
stringNoQuotes(Lexer* s)
{
    Token r {};

    u32 start = s->m_pos;
    u32 i = start;

    while (isalpha(s->m_sFile[i]))
        i++;

    r.sLiteral = {&s->m_sFile[start], i - start};

    if ("null" == r.sLiteral)
        r.type = Token::NULL_;
    else if ("false" == r.sLiteral)
        r.type = Token::FALSE_;
    else if ("true" == r.sLiteral)
        r.type = Token::TRUE_;
    else
        r.type = Token::IDENT;

    s->m_pos = i - 1;
    return r;
}

static Token
string(Lexer* s)
{
    Token r {};

    u32 start = s->m_pos;
    u32 i = start + 1;
    bool bEsc = false;

    while (s->m_sFile[i])
    {
        switch (s->m_sFile[i])
        {
            default:
                if (bEsc)
                    bEsc = false;
                break;

            case Token::EOF_:
                CERR("unterminated string\n");
                exit(1);

            case '\n':
                CERR("Unexpected newline within string");
                exit(1);

            case '\\':
                bEsc = !bEsc;
                break;

            case '"':
                if (!bEsc)
                    goto done;
                else
                    bEsc = false;
                break;
        }

        i++;
    }

done:

    r.type = Token::IDENT;
    r.sLiteral = {&s->m_sFile[start + 1], (i - start) - 1};

    s->m_pos = i;
    return r;
}

static Token
character(Lexer* s, Token::TYPE type)
{
    return {
        .type = type,
        .sLiteral = {&s->m_sFile[s->m_pos], 1}
    };
}

Token
Lexer::next()
{
    Token r {};

    if (m_pos >= m_sFile.m_size)
            return r;

    skipWhiteSpace(this);

    switch (m_sFile[m_pos])
    {
        default:
        /* solves bools and nulls */
        r = stringNoQuotes(this);
        break;

        case '-': case '+': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        r = number(this);
        break;

        case Token::QUOTE:
        r = string(this);
        break;

        case Token::COMMA:
        r = character(this, Token::COMMA);
        break;

        case Token::ASSIGN:
        r = character(this, Token::ASSIGN);
        break;

        case Token::LBRACE:
        r = character(this, Token::LBRACE);
        break;

        case Token::RBRACE:
        r = character(this, Token::RBRACE);
        break;

        case Token::RBRACKET:
        r = character(this, Token::RBRACKET);
        break;

        case Token::LBRACKET:
        r = character(this, Token::LBRACKET);
        break;

        case Token::EOF_:
        r.type = Token::EOF_;
        break;
    }

    m_pos++;
    return r;
}

} /* namespace json */
