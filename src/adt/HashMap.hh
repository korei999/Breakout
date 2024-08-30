#pragma once

#include "Array.hh"
#include "hash.hh"

namespace adt
{

constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR = 0.5;

template<typename T>
struct Bucket
{
    T data;
    bool bOccupied = false;
    bool bDeleted = false;
};

template<typename T>
struct HashMapRet
{
    T* pData;
    u64 hash;
    u32 idx;
    bool bInserted;
};

template<typename T>
struct HashMap
{
    Allocator* pAlloc;
    Array<Bucket<T>> aBuckets;
    f32 maxLoadFactor;
    u32 bucketCount = 0;

    HashMap() = default;
    HashMap(Allocator* pAllocator, u32 prealloc = SIZE_MIN)
        : pAlloc (pAllocator),
          aBuckets (pAllocator, prealloc),
          maxLoadFactor (HASHMAP_DEFAULT_LOAD_FACTOR) {}

    Bucket<T>& operator[](u32 i) { return aBuckets[i]; }
    const Bucket<T>& operator[](u32 i) const { return aBuckets[i]; }
};

template<typename T> inline f32 HashMapLoadFactor(HashMap<T>* s);
template<typename T> inline HashMapRet<T> HashMapInsert(HashMap<T>* s, const T& value);
template<typename T> inline HashMapRet<T> HashMapSearch(HashMap<T>* s, const T& value);
template<typename T> inline void HashMapRemove(HashMap<T>*s, u32 i);
template<typename T> inline HashMapRet<T> HashMapTryInsert(HashMap<T>* s, const T& value);
template<typename T> inline void HashMapDestroy(HashMap<T>* s);

template<typename T> inline void __HashMapRehash(HashMap<T>* s, u32 size);

template<typename T>
inline f32
HashMapLoadFactor(HashMap<T>* s)
{
    return f32(s->bucketCount) / f32(s->aBuckets.cap);
}

template<typename T>
inline HashMapRet<T>
HashMapInsert(HashMap<T>* s, const T& value)
{
    if (HashMapLoadFactor(s) >= s->maxLoadFactor)
        __HashMapRehash(s, s->aBuckets.cap * 2);

    u64 hash = hash::func(value);
    u32 idx = u32(hash % s->aBuckets.cap);

    while (s->aBuckets[idx].bOccupied)
    {
        idx++;
        if (idx >= s->aBuckets.cap)
            idx = 0;
    }

    s->aBuckets[idx].data = value;
    s->aBuckets[idx].bOccupied = true;
    s->aBuckets[idx].bDeleted = false;
    s->bucketCount++;

    return {
        .pData = &s->aBuckets[idx].data,
        .hash = hash,
        .idx = idx,
        .bInserted = true
    };
}

template<typename T>
inline HashMapRet<T>
HashMapSearch(HashMap<T>* s, const T& value)
{
    u64 hash = hash::func(value);
    u32 idx = u32(hash % s->aBuckets.cap);

    HashMapRet<T> ret;
    ret.hash = hash;
    ret.pData = nullptr;
    ret.bInserted = false;

    while (s->aBuckets[idx].bOccupied || s->aBuckets[idx].bDeleted)
    {
        if (s->aBuckets[idx].data == value)
        {
            ret.pData = &s->aBuckets[idx].data;
            break;
        }

        idx++;
        if (idx >= s->aBuckets.cap)
            idx = 0;
    }

    ret.idx = idx;
    return ret;
}

template<typename T>
inline void
HashMapRemove(HashMap<T>*s, u32 i)
{
    s->aBuckets[i].bDeleted = true;
    s->aBuckets[i].bOccupied = false;
}

template<typename T>
inline void
__HashMapRehash(HashMap<T>* s, u32 size)
{
    auto mNew = HashMap<T>(s->aBuckets.pAlloc, size);

    for (u32 i = 0; i < s->aBuckets.cap; i++)
        if (s->aBuckets[i].bOccupied)
            HashMapInsert(&mNew, s->aBuckets[i].data);

    HashMapDestroy(s);
    *s = mNew;
}

template<typename T>
inline HashMapRet<T>
HashMapTryInsert(HashMap<T>* s, const T& value)
{
    auto f = HashMapSearch(s, value);
    if (f.pData) return f;
    else return HashMapInsert(s, value);
}

template<typename T>
inline void
HashMapDestroy(HashMap<T>* s)
{
    ArrayDestroy(&s->aBuckets);
}

} /* namespace adt */
