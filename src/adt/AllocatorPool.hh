#pragma once

#include <assert.h>

#include "List.hh"
#include "Allocator.hh"
#include "ChunkAllocator.hh"

#include <stddef.h>

namespace adt
{

template<typename A>
struct AllocatorPool
{
    ChunkAllocator al;
    List<A> lAllocators;
    u32 size;

    constexpr AllocatorPool(u32 pre) : al(sizeof(ListNode<A>), sizeof(ListNode<A>) * pre), lAllocators(&al.base), size(0) {}
};

template<typename A>
inline Allocator*
AllocatorPoolGet(AllocatorPool<A>* s, u32 size)
{
    auto* pA = ListPushBack(&s->lAllocators, A(size));
    return (Allocator*)(&pA->data);
}

template<typename A>
inline void
AllocatorPoolGiveBack(AllocatorPool<A>* s, Allocator* p)
{
    auto* pNode = (ListNode<A>*)((u8*)(p) - offsetof(ListNode<A>, data));
    ListRemove(&s->lAllocators, pNode);
}

template<typename A>
inline void
AllocatorPoolFreeAll(AllocatorPool<A>* s)
{
    ChunkFreeAll(&s->al);
}

} /* namespace adt */
