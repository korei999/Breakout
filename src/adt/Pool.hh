#pragma once

#include "types.hh"
#include "guard.hh"
#include "Arr.hh"

#include <cstdio>
#include <cassert>
#include <limits>

#include <threads.h>

namespace adt
{

using PoolHnd = u32;

template<typename T>
struct PoolNode
{
    T data {};
    bool bDeleted {};
    /* TODO: ref counter? */
};

template<typename T, u32 CAP> struct Pool;

template<typename T, u32 CAP> inline s64 PoolNextIdx(Pool<T, CAP>* s, s64 i);
template<typename T, u32 CAP> inline s64 PoolPrevIdx(Pool<T, CAP>* s, s64 i);
template<typename T, u32 CAP> inline s64 PoolFirstIdx(Pool<T, CAP>* s);
template<typename T, u32 CAP> inline s64 PoolLastIdx(Pool<T, CAP>* s);

template<typename T, u32 CAP>
struct Pool
{
    Arr<PoolNode<T>, CAP> aData {};
    Arr<PoolHnd, CAP> aFreeIdxs {};
    u32 nOccupied {};
    mtx_t mtx;

    T&
    operator[](s64 i)
    {
        assert(!aData[i].bDeleted && "[MemPool]: accessing deleted node");
        return aData[i].data;
    }

    const T&
    operator[](s64 i) const
    {
        assert(!aData[i].bDeleted && "[MemPool]: accessing deleted node");
        return aData[i];
    }

    Pool() = default;
    Pool(INIT_FLAG e)
    { 
        if (e != INIT_FLAG::INIT) return;

        mtx_init(&mtx, mtx_plain);
    }

    struct It
    {
        Pool* s {};
        s64 i {};

        It(Pool* _self, s64 _i) : s(_self), i(_i) {}

        T& operator*() { return s->aData[i].data; }
        T* operator->() { return &s->aData[i].data; }

        It
        operator++()
        {
            i = PoolNextIdx(s, i);
            return {s, i};
        }

        It
        operator++(int)
        {
            s64 tmp = i;
            i = PoolNextIdx(s, i);
            return {s, tmp};
        }

        It
        operator--()
        {
            i = PoolPrevIdx(s, i);
            return {s, i};
        }

        It
        operator--(int)
        {
            s64 tmp = i;
            i = PoolPrevIdx(s, i);
            return {s, tmp};
        }

        friend bool operator==(It l, It r) { return l.i == r.i; }
        friend bool operator!=(It l, It r) { return l.i != r.i; }
    };

    It begin() { return {this, PoolFirstIdx(this)}; }
    It end() { return {this, PoolLastIdx(this) + 1}; }
    It rbegin() { return {this, PoolLastIdx(this)}; }
    It rend() { return {this, PoolFirstIdx(this) - 1}; }

    const It begin() const { return {this, PoolFirstIdx(this)}; }
    const It end() const { return {this, PoolLastIdx(this) + 1}; }
    const It rbegin() const { return {this, PoolLastIdx(this)}; }
    const It rend() const { return {this, PoolFirstIdx(this) - 1}; }
};

template<typename T, u32 CAP>
inline s64
PoolFirstIdx(Pool<T, CAP>* s)
{
    for (u32 i = 0; i < s->aData.size; ++i)
        if (!s->aData[i].bDeleted) return i;

    return s->aData.size;
}

template<typename T, u32 CAP>
inline s64
PoolLastIdx(Pool<T, CAP>* s)
{
    for (s64 i = s->aData.size - 1; i >= 0; --i)
        if (!s->aData[i].bDeleted) return i;

    return s->aData.size;
}

template<typename T, u32 CAP>
inline s64
PoolNextIdx(Pool<T, CAP>* s, s64 i)
{
    do ++i;
    while (i < s->aData.size && s->aData[i].bDeleted);

    return i;
}

template<typename T, u32 CAP>
inline s64
PoolPrevIdx(Pool<T, CAP>* s, s64 i)
{
    do --i;
    while (i >= 0 && s->aData[i].bDeleted);

    return i;
}

template<typename T, u32 CAP>
inline void
PoolDestroy(Pool<T, CAP>* s)
{
    mtx_destroy(&s->mtx);
}

template<typename T, u32 CAP>
[[nodiscard]] inline PoolHnd
PoolRent(Pool<T, CAP>* s)
{
    guard::Mtx lock(&s->mtx);

    PoolHnd ret = std::numeric_limits<PoolHnd>::max();

    if (s->nOccupied >= CAP)
    {
#ifndef NDEBUG
        fputs("[MemPool]: no free element, returning NPOS", stderr);
#endif
        return ret;
    }

    ++s->nOccupied;

    if (s->aFreeIdxs.size > 0) ret = *ArrPop(&s->aFreeIdxs);
    else ret = ArrFakePush(&s->aData);

    s->aData[ret].bDeleted = false;
    return ret;
}

template<typename T, u32 CAP>
inline void
PoolReturn(Pool<T, CAP>* s, PoolHnd hnd)
{
    guard::Mtx lock(&s->mtx);

    --s->nOccupied;
    assert(s->nOccupied < CAP && "[MemPool]: nothing to return");

    if (hnd == ArrSize(&s->aData) - 1) ArrFakePop(&s->aData);
    else
    {
        ArrPush(&s->aFreeIdxs, hnd);
        s->aData[hnd].bDeleted = true;
    }
}

template<typename T, u32 CAP>
[[nodiscard]] inline u32
PoolByteSize(const Pool<T, CAP>* const s)
{
    return sizeof(*s);
}

} /* namespace adt */
