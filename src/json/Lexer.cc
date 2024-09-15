#include "Lexer.hh"

#include "adt/file.hh"
#include "adt/logs.hh"

#include <ctype.h>

using namespace adt;

namespace json
{

void
LexerLoadFile(Lexer* s, adt::String path)
{
    Option<String> rs = file::load(s->pAlloc, path);
    if (!rs) LOG_FATAL("error opening file: '%.*s'\n", path.size, path.pData);

    s->sFile = rs.data;
    s->pos = 0;
}

void
LexerSkipWhiteSpace(Lexer* s)
{
    auto oneOf = [](char c) -> bool {
        const char skipChars[] = " \t\n\r";
        for (auto& s : skipChars)
            if (c == s) return true;

        return false;
    };

    while (s->pos < s->sFile.size && oneOf(s->sFile[s->pos]))
        s->pos++;
}

Token
LexerNumber(Lexer* s)
{
    Token r {};
    u32 start = s->pos;
    u32 i = start;

    while (isxdigit(s->sFile[i])       ||
                    s->sFile[i] == '.' ||
                    s->sFile[i] == '-' ||
                    s->sFile[i] == '+')
    {
        i++;
    }

    r.type = Token::NUMBER;
    r.sLiteral = {&s->sFile[start], i - start};
    
    s->pos = i - 1;
    return r;
}

Token
LexerStringNoQuotes(Lexer* s)
{
    Token r {};

    u32 start = s->pos;
    u32 i = start;

    while (isalpha(s->sFile[i]))
        i++;

    r.sLiteral = {&s->sFile[start], i - start};

    if ("null" == r.sLiteral)
        r.type = Token::NULL_;
    else if ("false" == r.sLiteral)
        r.type = Token::FALSE_;
    else if ("true" == r.sLiteral)
        r.type = Token::TRUE_;
    else
        r.type = Token::IDENT;

    s->pos = i - 1;
    return r;
}

Token
LexerString(Lexer* s)
{
    Token r {};

    u32 start = s->pos;
    u32 i = start + 1;
    bool bEsc = false;

    while (s->sFile[i])
    {
        switch (s->sFile[i])
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
    r.sLiteral = {&s->sFile[start + 1], (i - start) - 1};

    s->pos = i;
    return r;
}

Token
LexerChar(Lexer*s, enum Token::TYPE type)
{
    return {
        .type = type,
        .sLiteral = {&s->sFile[s->pos], 1}
    };
}

Token
LexerNext(Lexer* s)
{
    Token r {};

    if (s->pos >= s->sFile.size)
            return r;

    LexerSkipWhiteSpace(s);

    switch (s->sFile[s->pos])
    {
        default:
            /* solves bools and nulls */
            r = LexerStringNoQuotes(s);
            break;

        case '-':
        case '+':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            r = LexerNumber(s);
            break;

        case Token::QUOTE:
            r = LexerString(s);
            break;

        case Token::COMMA:
            r = LexerChar(s, Token::COMMA);
            break;

        case Token::ASSIGN:
            r = LexerChar(s, Token::ASSIGN);
            break;

        case Token::LBRACE:
            r = LexerChar(s, Token::LBRACE);
            break;

        case Token::RBRACE:
            r = LexerChar(s, Token::RBRACE);
            break;

        case Token::RBRACKET:
            r = LexerChar(s, Token::RBRACKET);
            break;

        case Token::LBRACKET:
            r = LexerChar(s, Token::LBRACKET);
            break;

        case Token::EOF_:
            r.type = Token::EOF_;
            break;
    }

    s->pos++;
    return r;
}

} /* namespace json */
