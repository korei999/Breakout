#pragma once

#include "adt/IAllocator.hh"
#include "adt/String.hh"

namespace json
{

enum RESULT : adt::u8 { OK, FAIL };

struct Token
{
    enum TYPE : adt::u8
    {
        LBRACE = '{',
        RBRACE = '}',
        LBRACKET = '[',
        RBRACKET = ']',
        QUOTE = '"',
        IDENT = 'I',
        NUMBER = 'N',
        TRUE_ = 'T',
        FALSE_ = 'F',
        NULL_ = 'n',
        ASSIGN = ':',
        COMMA = ',',
        DOT = '.',
        UNHANDLED = 'X',
        EOF_ = '\0',
    } type;
    adt::String sLiteral;
};

struct Lexer
{
    adt::String m_sFile;
    adt::u32 m_pos = 0;

    /* */

    Lexer() = default;

    void destroy(adt::IAllocator* pAlloc);
    RESULT loadFile(adt::IAllocator* pAlloc, adt::String path);
    Token next();
};

} /* namespace json */
