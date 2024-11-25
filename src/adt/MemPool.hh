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

using MemPoolHnd = u32;

template<typename T, u32 CAP>
struct MemPool
{
    Arr<T, CAP> aData;
    Arr<MemPoolHnd, CAP> aFreeIdxs;
    u32 nOccupied {};
    mtx_t mtx;

    T& operator[](u32 i) { assert(i < CAP && i < NPOS && "[MemPool]: out of range"); return aData[i]; }
    const T& operator[](u32 i) const { assert(i < CAP && i < NPOS && "[MemPool]: out of range"); return aData[i]; }

    MemPool() = default;
    MemPool(INIT_FLAG e)
    { 
        if (e != INIT_FLAG::INIT) return;

        mtx_init(&mtx, mtx_plain);
    }
};

template<typename T, u32 CAP>
inline void
MemPoolDestroy(MemPool<T, CAP>* s)
{
    mtx_destroy(&s->mtx);
}

template<typename T, u32 CAP>
[[nodiscard]] inline MemPoolHnd
MemPoolRent(MemPool<T, CAP>* s)
{
    guard::Mtx lock(&s->mtx);

    if (s->nOccupied >= CAP)
    {
#ifndef NDEBUG
        fputs("[MemPool]: no free element, returning NPOS", stderr);
#endif
        return std::numeric_limits<MemPoolHnd>::max();
    }

    ++s->nOccupied;

    if (s->aFreeIdxs.size > 0) return *ArrPop(&s->aFreeIdxs);
    else return ArrFakePush(&s->aData);
}

template<typename T, u32 CAP>
inline void
MemPoolReturn(MemPool<T, CAP>* s, MemPoolHnd hnd)
{
    guard::Mtx lock(&s->mtx);

    --s->nOccupied;
    assert(s->nOccupied < CAP && "[MemPool]: nothing to return");

    if (hnd == ArrSize(&s->aData) - 1) ArrFakePop(&s->aData);
    else ArrPush(&s->aFreeIdxs, hnd);
}

} /* namespace adt */
