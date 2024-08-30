#pragma once

#include "Allocator.hh"

#include <stdlib.h>

namespace adt
{

struct DefaultAllocator;

inline void* DefaultAlloc(DefaultAllocator* s, u64 mCount, u64 mSize);
inline void* DefaultRealloc(DefaultAllocator* s, void* p, u64 mCount, u64 mSize);
inline void DefaultFree(DefaultAllocator* s, void* p);

inline const AllocatorInterface __DefaultAllocatorVTable {
    .alloc = (decltype(AllocatorInterface::alloc))DefaultAlloc,
    .realloc = (decltype(AllocatorInterface::realloc))DefaultRealloc,
    .free = (decltype(AllocatorInterface::free))DefaultFree
};

struct DefaultAllocator
{
    Allocator base {};

    DefaultAllocator() : base {&__DefaultAllocatorVTable} {}
};

inline void*
DefaultAlloc([[maybe_unused]] DefaultAllocator* s, u64 mCount, u64 mSize)
{
    return ::calloc(mCount, mSize);
}

inline void*
DefaultRealloc([[maybe_unused]] DefaultAllocator* s, void* p, u64 mCount, u64 mSize)
{
    return ::reallocarray(p, mCount, mSize);
}

inline void
DefaultFree([[maybe_unused]] DefaultAllocator* s, void* p)
{
    ::free(p);
}

} /* namespace adt */
