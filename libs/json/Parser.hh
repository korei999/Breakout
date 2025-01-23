#pragma once

#include "Lexer.hh"
#include  "adt/Vec.hh"

#include <cassert>

namespace json
{

enum class TAG : adt::u8
{
    NULL_,
    STRING,
    LONG,
    DOUBLE,
    ARRAY,
    OBJECT,
    BOOL
};

inline adt::String
getTAGString(enum TAG t)
{
    constexpr adt::String TAGStrings[] {
        "NULL_", "STRING", "LONG", "DOUBLE", "ARRAY", "OBJECT", "BOOL"
    };

    return TAGStrings[static_cast<int>(t)];
}

struct Object;

union Val
{
    adt::null n;
    adt::String s;
    adt::s64 l;
    adt::f64 d;
    adt::VecBase<Object> a;
    adt::VecBase<Object> o;
    bool b;
};

struct TagVal
{
    TAG eTag {};
    union Val val {};
};

struct Object
{
    adt::String sKey {};
    TagVal tagVal {};

    /* */

    void
    freeData(adt::IAllocator* pAlloc)
    {
        pAlloc->free(tagVal.val.a.m_pData);
    }

    Object&
    operator[](adt::u32 i)
    {
        ADT_ASSERT((tagVal.eTag == TAG::OBJECT || tagVal.eTag == TAG::ARRAY),
            "not an ARRAY or OBJECT, tag is: '%s'", getTAGString(tagVal.eTag).data()
        );
        return tagVal.val.o[i];
    }

    Object&
    first()
    {
        ADT_ASSERT((tagVal.eTag == TAG::OBJECT || tagVal.eTag == TAG::ARRAY),
            "not an ARRAY or OBJECT, tag is: '%s'", getTAGString(tagVal.eTag).data()
        );
        return tagVal.val.o.first();
    }

    Object&
    last()
    {
        ADT_ASSERT((tagVal.eTag == TAG::OBJECT || tagVal.eTag == TAG::ARRAY),
            "not an ARRAY or OBJECT, tag is: '%s'", getTAGString(tagVal.eTag).data()
        );
        return tagVal.val.o.last();
    }

    adt::u32
    pushToArray(adt::IAllocator* pAlloc, const Object& o)
    {
        ADT_ASSERT(tagVal.eTag == TAG::ARRAY, "not an ARRAY, tag is: '%s'", getTAGString(tagVal.eTag).data());
        return tagVal.val.a.push(pAlloc, o);
    }

    adt::u32
    pushToObject(adt::IAllocator* pAlloc, const Object& o)
    {
        ADT_ASSERT(tagVal.eTag == TAG::OBJECT, "not an OBJECT, tag is: '%s'", getTAGString(tagVal.eTag).data());
        return tagVal.val.o.push(pAlloc, o);
    }
};

enum class STATUS : adt::u8 { OK, FAIL };
enum class TRAVERSAL_ORDER : adt::u8 { PRE, POST };

class Parser
{
    adt::IAllocator* m_pAlloc {};
    Lexer m_lex {};
    adt::VecBase<Object> m_aObjects {};
    Token m_tCurr {};
    Token m_tNext {};

    /* */

public:
    Parser() = default;

    /* */

    void destroy();

    STATUS parse(adt::IAllocator* pAlloc, adt::String sJson);

    void print(FILE* fp);

    /* if root json object consists of only one object return that, otherwise get array of root objects */
    adt::VecBase<Object>& getRoot();

    /* pfn returns true for early return */
    void traverse(bool (*pfn)(Object* p, void* pFnArgs), void* pArgs, TRAVERSAL_ORDER eOrder);

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

/* pfn returns true for early return */
void traverseNode(Object* pNode, bool (*pfn)(Object* pNode, void* pArgs), void* pArgs, TRAVERSAL_ORDER eOrder);
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
    assert(obj->tagVal.eTag == TAG::OBJECT);
    return obj->tagVal.val.o;
}

[[nodiscard]] inline adt::VecBase<Object>&
getArray(Object* obj)
{
    assert(obj->tagVal.eTag == TAG::ARRAY);
    return obj->tagVal.val.a;
}

[[nodiscard]] inline adt::s64
getLong(Object* obj)
{
    assert(obj->tagVal.eTag == TAG::LONG);
    return obj->tagVal.val.l;
}

[[nodiscard]] inline double
getDouble(Object* obj)
{
    assert(obj->tagVal.eTag == TAG::DOUBLE);
    return obj->tagVal.val.d;
}

[[nodiscard]] inline adt::String
getString(Object* obj)
{
    assert(obj->tagVal.eTag == TAG::STRING);
    return obj->tagVal.val.s;
}

[[nodiscard]] inline bool
getBool(Object* obj)
{
    assert(obj->tagVal.eTag == TAG::BOOL);
    return obj->tagVal.val.b;
}

[[nodiscard]] inline Object
makeObject(adt::IAllocator* pAlloc, adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::OBJECT, .val {.o {pAlloc}}}
    };
}

[[nodiscard]] inline Object
makeArray(adt::IAllocator* pAlloc, adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::ARRAY, .val {.a {pAlloc}}}
    };
}

[[nodiscard]] inline Object
makeNumber(adt::String key, adt::s64 l)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::LONG, .val {.l = l}}
    };
}

[[nodiscard]] inline Object
makeFloat(adt::String key, adt::f64 d)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::DOUBLE, .val {.d = d}}
    };
}

[[nodiscard]] inline Object
makeString(adt::String key, adt::String s)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::STRING, .val {.s = s}}
    };
}

[[nodiscard]] inline Object
makeBool(adt::String key, bool b)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::BOOL, .val {.b = b}}
    };
}

[[nodiscard]] inline Object
makeNull(adt::String key)
{
    return {
        .sKey = key,
        .tagVal {.eTag = TAG::NULL_, .val {.n = nullptr}}
    };
}

} /* namespace json */
