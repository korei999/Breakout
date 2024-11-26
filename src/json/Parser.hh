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
    adt::VecBase<Object> aObjects {};
    Token tCurr;
    Token tNext;

    Parser() = default;
    Parser(adt::Allocator* p) : pAlloc(p), l(p) {}
};

void ParserDestroy(Parser* s);
void ParserPrintNode(FILE* fp, Object* pNode, adt::String svEnd, int depth);
RESULT ParserLoad(Parser* s, adt::String path);
RESULT ParserParse(Parser* s);
RESULT ParserLoadParse(Parser* s, adt::String path);
void ParserPrint(Parser* s, FILE* fp);
void ParserTraverse(Parser* s, Object* pNode, bool (*pfn)(Object* p, void* a), void* args);

inline void
ParserTraverseAll(Parser* s, bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
{
    for (auto& obj : s->aObjects)
        ParserTraverse(s, &obj, pfn, pArgs);
}

/* Linear search inside JSON object. Returns nullptr if not found */
inline Object*
searchObject(adt::VecBase<Object>& aObj, adt::String svKey)
{
    for (adt::u32 i = 0; i < VecSize(&aObj); i++)
        if (aObj[i].svKey == svKey)
            return &aObj[i];

    return nullptr;
}

inline adt::VecBase<Object>&
getObject(Object* obj)
{
    return obj->tagVal.val.o;
}

inline adt::VecBase<Object>&
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
makeObject(adt::String key, adt::Allocator* pAlloc)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::OBJECT, .val {.o {pAlloc}}}
    };
}

inline Object
makeArray(adt::String key, adt::Allocator* pAlloc)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::ARRAY, .val {.a {pAlloc}}}
    };
}

inline Object
makeLong(adt::String key, long l)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::LONG, .val {.l = l}}
    };
}

inline Object
makeDouble(adt::String key, double d)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::DOUBLE, .val {.d = d}}
    };
}

inline Object
makeString(adt::String key, adt::String s)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::STRING, .val {.sv = s}}
    };
}

inline Object
makeBool(adt::String key, bool b)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::BOOL, .val {.b = b}}
    };
}

inline Object
makeNull(adt::String key)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::NULL_, .val {.n = nullptr}}
    };
}

inline adt::u32
pushToObject(Object* pObj, adt::Allocator* p, Object o)
{
    return adt::VecPush(&pObj->tagVal.val.o, p, o);
}

inline adt::u32
pushToArray(Object* pObj, adt::Allocator* p, Object o)
{
    return adt::VecPush(&pObj->tagVal.val.a, p, o);
}

/* if root json object consists of single object return that, otherwise return array of root objects */
inline adt::VecBase<Object>&
ParserGetHeadObj(Parser* s)
{
    assert(s->aObjects.size > 0 && "[Parser]: this json is empty");

    if (s->aObjects.size == 1) return getObject(&adt::VecFirst(&s->aObjects));
    else return s->aObjects;
}

} /* namespace json */
