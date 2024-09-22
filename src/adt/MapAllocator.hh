#pragma once

#include "Allocator.hh"
#include "HashMap.hh"
#include "adt/DefaultAllocator.hh"

#include <stdlib.h>

namespace adt
{

template<>
inline u64
hash::func<void* const>(void* const& x)
{
    return u64(x);
}

/* simple gen-purpose calloc/realloc/free/freeAll, while tracking allocations */
struct MapAllocator
{
    Allocator base;
    HashMap<void*> mAllocations;

    MapAllocator() = default;
    MapAllocator(u64 pre);
};

inline void*
MapAllocatorAlloc(MapAllocator* s, u64 mCount, u64 mSize)
{
    void* r = ::calloc(mCount, mSize);
    HashMapInsert(&s->mAllocations, r);
    return r;
}

inline void*
MapAllocatorRealloc(MapAllocator* s, void* p, u64 mCount, u64 mSize)
{
    HashMapRemove(&s->mAllocations, p);
    void* r = ::reallocarray(p, mCount, mSize);
    HashMapInsert(&s->mAllocations, r);
    return r;
}

inline void
MapAllocatorFree(MapAllocator* s, void* p)
{
    HashMapRemove(&s->mAllocations, p);
    ::free(p);
}

inline void
MapAllocatorFreeAll(MapAllocator* s)
{
    for (auto& b : s->mAllocations)
        ::free(b);

    HashMapDestroy(&s->mAllocations);
}

inline const AllocatorInterface __MapAllocatorVTable {
    .alloc = decltype(AllocatorInterface::alloc)(MapAllocatorAlloc),
    .realloc = decltype(AllocatorInterface::realloc)(MapAllocatorRealloc),
    .free = decltype(AllocatorInterface::free)(MapAllocatorFree),
    .freeAll = decltype(AllocatorInterface::freeAll)(MapAllocatorFreeAll),
};

inline
MapAllocator::MapAllocator(u64 pre)
    : base {&__MapAllocatorVTable}, mAllocations(&in_OsAllocator.base) {}

inline void* alloc(MapAllocator* s, u64 mCount, u64 mSize) { return MapAllocatorAlloc(s, mCount, mSize); }
inline void* realloc(MapAllocator* s, void* p, u64 mCount, u64 mSize) { return MapAllocatorRealloc(s, p, mCount, mSize); }
inline void free(MapAllocator* s, void* p) { MapAllocatorFree(s, p); }
inline void freeAll(MapAllocator* s) { MapAllocatorFreeAll(s); }

} /* namespace adt */
