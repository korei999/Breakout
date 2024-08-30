#pragma once

#include <threads.h>

#include "Arena.hh"

namespace adt
{

struct AtomicArena
{
    Arena arena;
    mtx_t mtx;

    AtomicArena() = default;
    AtomicArena(u32 blockCap);
};

inline void*
AtomicArenaAlloc(AtomicArena* s, u64 mCount, u64 mSize)
{
    mtx_lock(&s->mtx);
    auto* r = ArenaAlloc(&s->arena, mCount, mSize);
    mtx_unlock(&s->mtx);

    return r;
}

inline void*
AtomicArenaRealloc(AtomicArena* s, void* p, u64 mCount, u64 mSize)
{
    mtx_lock(&s->mtx);
    auto* r = ArenaRealloc(&s->arena, p, mCount, mSize);
    mtx_unlock(&s->mtx);

    return r;
}

inline void
AtomicArenaFree([[maybe_unused]] AtomicArena* s, [[maybe_unused]] void* p)
{
    /* no individual frees */
}

inline const AllocatorInterface __AtomicArenaAllocatorVTable {
    .alloc = decltype(AllocatorInterface::alloc)(AtomicArenaAlloc),
    .realloc = decltype(AllocatorInterface::realloc)(AtomicArenaRealloc),
    .free = decltype(AllocatorInterface::free)(AtomicArenaFree)
};

inline
AtomicArena::AtomicArena(u32 blockCap)
    : arena (blockCap)
{
    arena.base = {&__AtomicArenaAllocatorVTable};
    mtx_init(&mtx, mtx_plain);
}

inline void
AtomicArenaFreeAll(AtomicArena* s)
{
    ArenaFreeAll(&s->arena);
    mtx_destroy(&s->mtx);
}

} /* namespace adt */
