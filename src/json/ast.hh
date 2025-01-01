#pragma once

#include "adt/String.hh"
#include "adt/Vec.hh"

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

static const char* TAGStrings[] {
    "NULL_", "STRING", "LONG", "DOUBLE", "ARRAY", "OBJECT", "BOOL"
};

inline const char*
getTAGString(enum TAG t)
{
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
    enum TAG tag;
    union Val val;
};

struct Object
{
    adt::String sKey;
    TagVal tagVal;

    /* */

    Object&
    operator[](adt::u32 i)
    {
        assert(tagVal.tag == TAG::OBJECT || tagVal.tag == TAG::ARRAY && "[json]: using operator[] on non ARRAY or OBJECT");
        return tagVal.val.o[i];
    }

    Object&
    first()
    {
        assert(tagVal.tag == TAG::OBJECT || tagVal.tag == TAG::ARRAY && "[json]: last() on non ARRAY or OBJECT");
        return tagVal.val.o.first();
    }

    Object&
    last()
    {
        assert(tagVal.tag == TAG::OBJECT || tagVal.tag == TAG::ARRAY && "[json]: last() on non ARRAY or OBJECT");
        return tagVal.val.o.last();
    }

    adt::u32
    pushToArray(adt::IAllocator* pAlloc, const Object& o)
    {
        assert(tagVal.tag == TAG::ARRAY && "[json]: this object is not tagged as ARRAY");
        return tagVal.val.a.push(pAlloc, o);
    }

    adt::u32
    pushToObject(adt::IAllocator* pAlloc, const Object& o)
    {
        assert(tagVal.tag == TAG::OBJECT && "[json]: this object is not tagged as OBJECT");
        return tagVal.val.o.push(pAlloc, o);
    }
};

} /* namespace json */
