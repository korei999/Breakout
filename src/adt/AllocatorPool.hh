#pragma once

#include "List.hh"
#include "Allocator.hh"
#include "ChunkAllocator.hh"

#include <cassert>
#include <cstddef>

namespace adt
{

template<typename A>
struct AllocatorPool
{
    ChunkAllocator al {};
    ListBase<A> lAllocators {};

    constexpr AllocatorPool(u32 pre) : al(sizeof(ListNode<A>), sizeof(ListNode<A>) * pre) {}
};

template<typename A>
inline Allocator*
AllocatorPoolGet(AllocatorPool<A>* s, u32 size)
{
    auto* pA = ListPushBack(&s->lAllocators, &s->al.base, A(size));
    return (Allocator*)(&pA->data);
}

template<typename A>
inline void
AllocatorPoolGiveBack(AllocatorPool<A>* s, Allocator* p)
{
    auto* pNode = (ListNode<A>*)((u8*)p - offsetof(ListNode<A>, data));
    freeAll(pNode->data);
    ListRemove(&s->lAllocators, pNode);
}

template<typename A>
inline void
AllocatorPoolFreeAll(AllocatorPool<A>* s)
{
    for (auto& a : s->lAllocators)
        freeAll(&a);

    ChunkFreeAll(&s->al);
}

} /* namespace adt */
