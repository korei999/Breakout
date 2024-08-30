#pragma once

#include "Lexer.hh"
#include "ast.hh"

namespace json
{


struct Parser
{
    adt::Allocator* pAlloc;
    Lexer l;
    adt::String sName;
    Object* pHead;
    Token tCurr;
    Token tNext;

    Parser() = default;
    Parser(adt::Allocator* p) : pAlloc(p), l(p) {}
};

void ParserPrintNode(Object* pNode, adt::String svEnd, int depth);
void ParserLoad(Parser* s, adt::String path);
void ParserParse(Parser* s);
void ParserPrint(Parser* s);
void ParserTraverse(Parser* s, Object* pNode, bool (*pfn)(Object* p, void* a), void* args);
inline void ParserTraverse(Parser* s, bool (*pfn)(Object* p, void* a), void* args) { ParserTraverse(s, s->pHead, pfn, args); }
inline Object* ParserGetHeadObj(Parser* s) { return s->pHead; }

/* Linear search inside JSON object. Returns nullptr if not found */
inline Object*
searchObject(adt::Array<Object>& aObj, adt::String svKey)
{
    for (adt::u32 i = 0; i < aObj.size; i++)
        if (aObj[i].svKey == svKey)
            return &aObj[i];

    return nullptr;
}

inline adt::Array<Object>&
getObject(Object* obj)
{
    return obj->tagVal.val.o;
}

inline adt::Array<Object>&
getArray(Object* obj)
{
    return obj->tagVal.val.a;
}

inline long
getLong(Object* obj)
{
    return obj->tagVal.val.l;
}

inline double
getDouble(Object* obj)
{
    return obj->tagVal.val.d;
}

inline adt::String
getString(Object* obj)
{
    return obj->tagVal.val.sv;
}

inline bool
getBool(Object* obj)
{
    return obj->tagVal.val.b;
}

inline Object
putObject(adt::String key, adt::Allocator* pAlloc)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::OBJECT, .val {.o {pAlloc}}}
    };
}

inline Object
putArray(adt::String key, adt::Allocator* pAlloc)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::ARRAY, .val {.a {pAlloc}}}
    };
}

inline Object
putLong(adt::String key, long l)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::LONG, .val {.l = l}}
    };
}

inline Object
putDouble(adt::String key, double d)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::DOUBLE, .val {.d = d}}
    };
}

inline Object
putString(adt::String key, adt::String s)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::STRING, .val {.sv = s}}
    };
}

inline Object
putBool(adt::String key, bool b)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::BOOL, .val {.b = b}}
    };
}

inline Object
putNull(adt::String key)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::NULL_, .val {.n = nullptr}}
    };
}

inline void
pushToObject(Object* pObj, Object o)
{
    adt::ArrayPush(&pObj->tagVal.val.o, o);
}

inline void
pushToArray(Object* pObj, Object o)
{
    adt::ArrayPush(&pObj->tagVal.val.a, o);
}

} /* namespace json */
