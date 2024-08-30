#pragma once

#include "Allocator.hh"

#include <assert.h>
#include <string.h>

namespace adt
{

struct FixedAllocator
{
    Allocator base {};
    u8* pMemBuffer = nullptr;
    u64 size = 0;
    u64 cap = 0;
    void* pLastAlloc = nullptr;

    constexpr FixedAllocator() = default;
    constexpr FixedAllocator(void* pMemory, u64 capacity);
};

constexpr void* FixedAllocatorAlloc(FixedAllocator* s, u64 mCount, u64 mSize);
constexpr void* FixedAllocatorRealloc(FixedAllocator* s, void* p, u64 mCount, u64 mSize);
constexpr void FixedAllocatorFree([[maybe_unused]] FixedAllocator* s, [[maybe_unused]] void* p);

constexpr void*
FixedAllocatorAlloc(FixedAllocator* s, u64 mCount, u64 mSize)
{
    u64 aligned = align8(mCount * mSize);
    void* ret = &s->pMemBuffer[s->size];
    s->size += aligned;
    s->pLastAlloc = ret;

    assert(s->size < s->cap && "Out of memory");

    return ret;
}

constexpr void*
FixedAllocatorRealloc(FixedAllocator* s, void* p, u64 mCount, u64 mSize)
{
    void* ret = nullptr;
    u64 aligned = align8(mCount * mSize);

    if (p == s->pLastAlloc)
    {
        s->size -= (u8*)&s->pMemBuffer[s->size] - (u8*)p;
        s->size += aligned;

        return p;
    }
    else
    {
        ret = &s->pMemBuffer[s->size];
        s->pLastAlloc = ret;
        memcpy(ret, p, aligned);
        s->size += aligned;
    }

    return ret;
}

constexpr void
FixedAllocatorFree([[maybe_unused]] FixedAllocator* s, [[maybe_unused]] void* p)
{
    /* not needed since stack memory should be used as a buffer */
}

inline const AllocatorInterface __FixedAllocatorVTable {
    .alloc = decltype(AllocatorInterface::alloc)(FixedAllocatorAlloc),
    .realloc = decltype(AllocatorInterface::realloc)(FixedAllocatorRealloc),
    .free = decltype(AllocatorInterface::free)(FixedAllocatorFree)
};

constexpr FixedAllocator::FixedAllocator(void* pMemory, u64 capacity)
    : base {.pVTable = &__FixedAllocatorVTable}, pMemBuffer {(u8*)pMemory}, cap {capacity} {}

} /* namespace adt */
