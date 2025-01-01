#pragma once

#include "Lexer.hh"
#include "ast.hh"

#include <cassert>

namespace json
{

enum class STATUS : adt::u8 { OK, FAIL };

class Parser
{
    adt::IAllocator* m_pAlloc;
    Lexer m_lex;
    adt::VecBase<Object> m_aObjects {};
    Token m_tCurr;
    Token m_tNext;

    /* */

public:
    Parser() = default;
    Parser(adt::IAllocator* p) : m_pAlloc(p) {}

    /* */

    void destroy();
    STATUS parse(adt::String sJson);
    void print(FILE* fp);
    void traverse(Object* pNode, bool (*pfn)(Object* pNode, void* pArgs), void* pArgs);
    /* if root json object consists of only one object return that, otherwise get array of root objects */
    adt::VecBase<Object>& getRoot();

    void
    traverseAll(bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
    {
        for (auto& obj : m_aObjects) traverse(&obj, pfn, pArgs);
    }

private:
    STATUS parseNode(Object* pNode);
    STATUS parseObject(Object* pNode);
    STATUS parseArray(Object* pNode); /* arrays are same as objects */
    void parseIdent(TagVal* pTV);
    void parseString(TagVal* pTV);
    void parseNumber(TagVal* pTV);
    void parseFloat(TagVal* pTV);
    STATUS expect(TOKEN_TYPE t);
    STATUS expectNot(TOKEN_TYPE t);
    STATUS printNodeError();
    void next();
};

void printNode(FILE* fp, Object* pNode, adt::String sEnd = "", int depth = 0);

/* Linear search inside JSON object. Returns nullptr if not found */
[[nodiscard]] inline Object*
searchObject(adt::VecBase<Object>& aObj, adt::String sKey)
{
    for (adt::u32 i = 0; i < aObj.getSize(); i++)
        if (aObj[i].sKey == sKey)
            return &aObj[i];

    return nullptr;
}

[[nodiscard]] inline adt::VecBase<Object>&
getObject(Object* obj)
{
    assert(obj->tagVal.tag == TAG::OBJECT);
    return obj->tagVal.val.o;
}

[[nodiscard]] inline adt::VecBase<Object>&
getArray(Object* obj)
{
    assert(obj->tagVal.tag == TAG::ARRAY);
    return obj->tagVal.val.a;
}

[[nodiscard]] inline long
getLong(Object* obj)
{
    assert(obj->tagVal.tag == TAG::LONG);
    return obj->tagVal.val.l;
}

[[nodiscard]] inline double
getDouble(Object* obj)
{
    assert(obj->tagVal.tag == TAG::DOUBLE);
    return obj->tagVal.val.d;
}

[[nodiscard]] inline adt::String
getString(Object* obj)
{
    assert(obj->tagVal.tag == TAG::STRING);
    return obj->tagVal.val.s;
}

[[nodiscard]] inline bool
getBool(Object* obj)
{
    assert(obj->tagVal.tag == TAG::BOOL);
    return obj->tagVal.val.b;
}

[[nodiscard]] inline Object
makeObject(adt::IAllocator* pAlloc, adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::OBJECT, .val {.o {pAlloc}}}
    };
}

[[nodiscard]] inline Object
makeArray(adt::IAllocator* pAlloc, adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::ARRAY, .val {.a {pAlloc}}}
    };
}

[[nodiscard]] inline Object
makeNumber(adt::String key, adt::s64 l)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::LONG, .val {.l = l}}
    };
}

[[nodiscard]] inline Object
makeFloat(adt::String key, adt::f64 d)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::DOUBLE, .val {.d = d}}
    };
}

[[nodiscard]] inline Object
makeString(adt::String key, adt::String s)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::STRING, .val {.s = s}}
    };
}

[[nodiscard]] inline Object
makeBool(adt::String key, bool b)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::BOOL, .val {.b = b}}
    };
}

[[nodiscard]] inline Object
makeNull(adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.tag = TAG::NULL_, .val {.n = nullptr}}
    };
}

} /* namespace json */
