#pragma once

#include "Lexer.hh"
#include "ast.hh"

#include <cassert>

namespace json
{

struct Parser
{
    adt::IAllocator* m_pAlloc;
    Lexer m_lex;
    adt::String m_sName;
    adt::VecBase<Object> m_aObjects {};
    Token m_tCurr;
    Token m_tNext;

    /* */

    Parser() = default;
    Parser(adt::IAllocator* p) : m_pAlloc(p) {}

    void destroy();
    RESULT load(adt::String path);
    RESULT parse();
    RESULT loadParse(adt::String path);
    void print(FILE* fp);
    void traverse(Object* pNode, bool (*pfn)(Object* pNode, void* pArgs), void* pArgs);
    /* if root json object consists of only one object return that, otherwise get array of root objects */
    adt::VecBase<Object>& getRoot();

    void
    traverseAll(bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
    {
        for (auto& obj : m_aObjects) traverse(&obj, pfn, pArgs);
    }
};

void printNode(FILE* fp, Object* pNode, adt::String svEnd, int depth);

/* Linear search inside JSON object. Returns nullptr if not found */
inline Object*
searchObject(adt::VecBase<Object>& aObj, adt::String svKey)
{
    for (adt::u32 i = 0; i < aObj.getSize(); i++)
        if (aObj[i].svKey == svKey)
            return &aObj[i];

    return nullptr;
}

inline adt::VecBase<Object>&
getObject(Object* obj)
{
    assert(obj->tagVal.tag == TAG::OBJECT);
    return obj->tagVal.val.o;
}

inline adt::VecBase<Object>&
getArray(Object* obj)
{
    assert(obj->tagVal.tag == TAG::ARRAY);
    return obj->tagVal.val.a;
}

inline long
getLong(Object* obj)
{
    assert(obj->tagVal.tag == TAG::LONG);
    return obj->tagVal.val.l;
}

inline double
getDouble(Object* obj)
{
    assert(obj->tagVal.tag == TAG::DOUBLE);
    return obj->tagVal.val.d;
}

inline adt::String
getString(Object* obj)
{
    assert(obj->tagVal.tag == TAG::STRING);
    return obj->tagVal.val.sv;
}

inline bool
getBool(Object* obj)
{
    assert(obj->tagVal.tag == TAG::BOOL);
    return obj->tagVal.val.b;
}

inline Object
makeObject(adt::String key, adt::IAllocator* pAlloc)
{
    return {
        .svKey = key,
        .tagVal {.tag = TAG::OBJECT, .val {.o {pAlloc}}}
    };
}

inline Object
makeArray(adt::String key, adt::IAllocator* pAlloc)
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
pushToObject(Object* pObj, adt::IAllocator* p, Object o)
{
    return pObj->tagVal.val.o.push(p, o);
}

inline adt::u32
pushToArray(Object* pObj, adt::IAllocator* p, Object o)
{
    return pObj->tagVal.val.a.push(p, o);
}

} /* namespace json */
