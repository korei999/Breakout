#pragma once

#include <assert.h>

#include "Allocator.hh"

namespace adt
{

template<typename A, u32 MAX>
struct AllocatorPool
{
    A aAllocators[MAX];
    u32 size;
    u32 cap;

    AllocatorPool() : size (0), cap (MAX) {}
};

template<typename A, u32 MAX>
Allocator*
AllocatorPoolGet(AllocatorPool<A, MAX>* s, u32 size)
{
    assert(s->size < s->cap && "size reached cap");
    s->aAllocators[s->size++] = A (size);
    return &s->aAllocators[s->size - 1].base;
}

} /* namespace adt */
