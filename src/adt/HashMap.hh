#pragma once

#include "Vec.hh"
#include "hash.hh"

namespace adt
{

constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR = 0.5;
constexpr f32 HASHMAP_DEFAULT_LOAD_FACTOR_INV = 1.0 / HASHMAP_DEFAULT_LOAD_FACTOR;

template<typename T> struct HashMapBase;

template<typename T> inline void __HashMapRehash(HashMapBase<T>* s, Allocator* p, u32 size);

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
    /*u32 idx;*/ /* can be calculated from pData */
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
struct HashMapBase
{
    VecBase<Bucket<T>> aBuckets;
    f32 maxLoadFactor;
    u32 bucketCount = 0;

    HashMapBase() = default;
    HashMapBase(Allocator* pAllocator, u32 prealloc = SIZE_MIN)
        : aBuckets(pAllocator, prealloc),
          maxLoadFactor(HASHMAP_DEFAULT_LOAD_FACTOR) {}

    struct It
    {
        HashMapBase* s {};
        u32 i = 0;

        It(HashMapBase* _s, u32 _i) : s {_s}, i {_i} {}

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

    const It begin() const { return {this, HashMapFirstI(this)}; }
    const It end() const { return {this, NPOS}; }
};

template<typename T>
inline u32
HashMapIdx(HashMapBase<T>* s, T* p)
{
    return p - VecData(&s->aBuckets);
}

template<typename T>
inline u32
HashMapFirstI(HashMapBase<T>* s)
{
    u32 i = 0;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied)
        i++;

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename T>
inline u32
HashMapNextI(HashMapBase<T>* s, u32 i)
{
    ++i;
    while (i < VecCap(&s->aBuckets) && !s->aBuckets[i].bOccupied)
        i++;

    if (i >= VecCap(&s->aBuckets)) i = NPOS;

    return i;
}

template<typename T>
inline f32
HashMapLoadFactor(HashMapBase<T>* s)
{
    return f32(s->bucketCount) / f32(VecCap(&s->aBuckets));
}

template<typename T>
inline HashMapResult<T>
HashMapInsert(HashMapBase<T>* s, Allocator* p, const T& value)
{
    if (VecCap(&s->aBuckets) == 0) *s = {p};

    if (HashMapLoadFactor(s) >= s->maxLoadFactor)
        __HashMapRehash(s, p, VecCap(&s->aBuckets) * 2);

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
        /*.idx = idx,*/
        .bInserted = true
    };
}

template<typename T>
inline HashMapResult<T>
HashMapSearch(HashMapBase<T>* s, const T& value)
{
    assert(s && "HashMapBase: map is nullptr");

    if (s->bucketCount == 0) return {};

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

    /*ret.idx = idx;*/
    return ret;
}

template<typename T>
inline void
HashMapRemove(HashMapBase<T>*s, u32 i)
{
    s->aBuckets[i].bDeleted = true;
    s->aBuckets[i].bOccupied = false;

    s->bucketCount--;
}

template<typename T>
inline void
HashMapRemove(HashMapBase<T>*s, const T& x)
{
    auto f = HashMapSearch(s, x);
    HashMapRemove(s, f.idx);
}

template<typename T>
inline void
__HashMapRehash(HashMapBase<T>* s, Allocator* p, u32 size)
{
    auto mNew = HashMapBase<T>(p, size);

    for (u32 i = 0; i < VecCap(&s->aBuckets); i++)
        if (s->aBuckets[i].bOccupied)
            HashMapInsert(&mNew, p, s->aBuckets[i].data);

    HashMapDestroy(s, p);
    *s = mNew;
}

template<typename T>
inline HashMapResult<T>
HashMapTryInsert(HashMapBase<T>* s, Allocator* p, const T& value)
{
    auto f = HashMapSearch(s, value);
    if (f) return f;
    else return HashMapInsert(s, p, value);
}

template<typename T>
inline void
HashMapDestroy(HashMapBase<T>* s, Allocator* p)
{
    VecDestroy(&s->aBuckets, p);
}

template<typename T>
struct HashMap
{
    HashMapBase<T> base {};
    Allocator* pA {};

    HashMap() = default;
    HashMap(Allocator* _pA, u32 prealloc = SIZE_MIN)
        : base(_pA, prealloc), pA(_pA) {}

    HashMapBase<T>::It begin() { return base.begin(); }
    HashMapBase<T>::It end() { return base.end(); }
    HashMapBase<T>::It rbegin() { return base.rbegin(); }
    HashMapBase<T>::It rend() { return rend(); }

    const HashMapBase<T>::It begin() const { return begin(); }
    const HashMapBase<T>::It end() const { return end(); }
    const HashMapBase<T>::It rbegin() const { return rbegin(); }
    const HashMapBase<T>::It rend() const { return rend(); }
};

template<typename T>
inline u32
HashMapIdx(HashMap<T>* s, T* p)
{
    return HashMapIdx(&s->base, p);
}

template<typename T>
inline u32
HashMapFirstI(HashMap<T>* s)
{
    return HashMapFirstI(&s->base);
}

template<typename T>
inline u32
HashMapNextI(HashMap<T>* s, u32 i)
{
    return HashMapNextI(&s->base, i);
}

template<typename T>
inline f32
HashMapLoadFactor(HashMap<T>* s)
{
    return HashMapLoadFactor(&s->base);
}

template<typename T>
inline HashMapResult<T>
HashMapInsert(HashMap<T>* s, const T& value)
{
    return HashMapInsert(&s->base, s->pA, value);
}

template<typename T>
inline HashMapResult<T>
HashMapSearch(HashMap<T>* s, const T& value)
{
    return HashMapSearch(&s->base, value);
}

template<typename T>
inline void
HashMapRemove(HashMap<T>*s, u32 i)
{
    HashMapRemove(&s->base, i);
}

template<typename T>
inline void
HashMapRemove(HashMap<T>*s, const T& x)
{
    HashMapRemove(&s->base, x);
}

template<typename T>
inline void
__HashMapRehash(HashMap<T>* s, u32 size)
{
    __HashMapRehash(&s->base, s->pA, size);
}

template<typename T>
inline HashMapResult<T>
HashMapTryInsert(HashMap<T>* s, const T& value)
{
    return HashMapTryInsert(&s->base, s->pA, value);
}

template<typename T>
inline void
HashMapDestroy(HashMap<T>* s)
{
    HashMapDestroy(&s->base, s->pA);
}


} /* namespace adt */
