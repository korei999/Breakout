#pragma once

#include "Vec.hh"
#include "hash.hh"

namespace adt
{

constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR = 0.5;
constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR_INV = 1.0 / HASHMAP_DEFAULT_LOAD_FACTOR;

template<typename T> struct Bucket;
template<typename T> struct HashMapResult;
template<typename T> struct HashMap;

template<typename T> inline u32 HashMapFirstI(HashMap<T>* s);
template<typename T> inline u32 HashMapNextI(HashMap<T>* s, u32 i);
template<typename T> inline f32 HashMapLoadFactor(HashMap<T>* s);
template<typename T> inline HashMapResult<T> HashMapInsert(HashMap<T>* s, const T& value);
template<typename T> inline HashMapResult<T> HashMapSearch(HashMap<T>* s, const T& value);
template<typename T> inline void HashMapRemove(HashMap<T>*s, u32 i);
template<typename T> inline void HashMapRemove(HashMap<T>*s, const T& x);
template<typename T> inline HashMapResult<T> HashMapTryInsert(HashMap<T>* s, const T& value);
template<typename T> inline void HashMapDestroy(HashMap<T>* s);

template<typename T> inline void __HashMapRehash(HashMap<T>* s, u32 size);

template<typename T>
struct Bucket
{
    T data;
    bool bOccupied = false;
    bool bDeleted = false;
};

/* custom return type for insert/search operations */
template<typename T>
struct HashMapResult
{
    T* pData;
    u64 hash;
    u32 idx;
    bool bInserted;

    constexpr explicit operator bool() const
    {
        return this->pData != nullptr;
    }
};

/* Value is the key in this map.
 * For key/value pairs, use struct { KEY k; VALUE v; }, add `u64 adt::hash::func(const struct&)`
 * and* bool operator==(const struct& l, const struct& r). */
template<typename T>
struct HashMap
{
    Vec<Bucket<T>> aBuckets;
    f32 maxLoadFactor;
    u32 bucketCount = 0;

    HashMap() = default;
    HashMap(Allocator* pAllocator, u32 prealloc = SIZE_MIN)
        : aBuckets (pAllocator, prealloc),
          maxLoadFactor (HASHMAP_DEFAULT_LOAD_FACTOR) {}

    Bucket<T>& operator[](u32 i) { return aBuckets[i]; }
    const Bucket<T>& operator[](u32 i) const { return aBuckets[i]; }

    struct It
    {
        HashMap* s {};
        u32 i = 0;

        It(HashMap* _s, u32 _i) : s {_s}, i {_i} {}

        T& operator*() { return s->aBuckets[i].data; }
        T* operator->() { return &s->aBuckets[i].data; }

        It operator++()
        {
            i = HashMapNextI(s, i);
            return {s, i};
        }
        It operator++(int) { T* tmp = s++; return tmp; }

        friend constexpr bool operator==(const It& l, const It& r) { return l.i == r.i; }
        friend constexpr bool operator!=(const It& l, const It& r) { return l.i != r.i; }
    };

    It begin() { return {this, HashMapFirstI(this)}; }
    It end() { return {this, NPOS}; }

    const It begin() const { return begin(); }
    const It end() const { return end(); }
};

template<typename T>
inline u32
HashMapFirstI(HashMap<T>* s)
{
    u32 i = 0;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied)
        i++;

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename T>
inline u32
HashMapNextI(HashMap<T>* s, u32 i)
{
    ++i;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied)
        i++;

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename T>
inline f32
HashMapLoadFactor(HashMap<T>* s)
{
    return f32(s->bucketCount) / f32(VecCap(&s->aBuckets));
}

template<typename T>
inline HashMapResult<T>
HashMapInsert(HashMap<T>* s, const T& value)
{
    if (HashMapLoadFactor(s) >= s->maxLoadFactor)
        __HashMapRehash(s, VecCap(&s->aBuckets) * 2);

    u64 hash = hash::func(value);
    u32 idx = u32(hash % VecCap(&s->aBuckets));

    while (s->aBuckets[idx].bOccupied)
    {
        idx++;
        if (idx >= VecCap(&s->aBuckets))
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
inline HashMapResult<T>
HashMapSearch(HashMap<T>* s, const T& value)
{
    u64 hash = hash::func(value);
    u32 idx = u32(hash % VecCap(&s->aBuckets));

    HashMapResult<T> ret;
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
        if (idx >= VecCap(&s->aBuckets)) idx = 0;
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

    s->bucketCount--;
}

template<typename T>
inline void
HashMapRemove(HashMap<T>*s, const T& x)
{
    auto f = HashMapSearch(s, x);
    HashMapRemove(s, f.idx);
}

template<typename T>
inline void
__HashMapRehash(HashMap<T>* s, u32 size)
{
    auto mNew = HashMap<T>(s->aBuckets.pAlloc, size);

    for (u32 i = 0; i < VecCap(&s->aBuckets); i++)
        if (s->aBuckets[i].bOccupied)
            HashMapInsert(&mNew, s->aBuckets[i].data);

    HashMapDestroy(s);
    *s = mNew;
}

template<typename T>
inline HashMapResult<T>
HashMapTryInsert(HashMap<T>* s, const T& value)
{
    auto f = HashMapSearch(s, value);
    if (f) return f;
    else return HashMapInsert(s, value);
}

template<typename T>
inline void
HashMapDestroy(HashMap<T>* s)
{
    VecDestroy(&s->aBuckets);
}

} /* namespace adt */
