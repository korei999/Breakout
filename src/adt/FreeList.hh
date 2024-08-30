#pragma once

#include "RBTree.hh"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

namespace adt
{

#define ADT_FREELIST_FOREACH(A, IT) for (FreeListBlock* IT = (A)->pBlocks; IT; (IT) = (IT)->pNext)
#define ADT_FREELIST_FOREACH_SAFE(A, IT, TMP) for (FreeListBlock* IT = (A)->pBlocks, * TMP = nullptr; IT && ((TMP) = (IT)->pNext, true); (IT) = (TMP))

struct FreeList;
struct FreeListBlock;
struct FreeListNode;

inline void FreeListFree(FreeList* s, void* p);

inline FreeListBlock* FreeListFindBlock(FreeList* s, RBNode<FreeListNode>* p);
inline FreeListBlock* FreeListNewBlock(FreeList *s, u64 size);
inline RBNode<FreeListNode>* FreeListNewBlockToTheEnd(FreeList* s, u64 size);

struct FreeListNode
{
    u64 size = 0; /* TODO: hide bFreed in the sign bit */
    bool bFreed;
    RBNode<FreeListNode>* prev;
    RBNode<FreeListNode>* next;
    u8 pData[];
};

constexpr u64
FreeListNodeSize(const FreeListNode* p)
{
    return (0UL << 63) & p->size;
}

constexpr bool
FreeListNodeIsFree(const FreeListNode* p)
{
    return u64(p->size) >> 63;
}

template<>
constexpr s64
utils::compare(const FreeListNode& l, const FreeListNode& r)
{
    return l.size - r.size;
}

struct FreeListBlock
{
    u64 size = 0;
    FreeListBlock* pNext;
    u8 pMem[];
};

/* Red-Black tree Free list (best fit) allocator implementation */
struct FreeList
{
    Allocator base {};
    FreeListBlock* pBlocks = nullptr;
    // FreeListBlock* pBlocksTail = nullptr; /* TODO: track tail */
    RBTree<FreeListNode> tree {};

    FreeList() = default;
    FreeList(u64 prealloc);
};

inline void*
FreeListAlloc(FreeList* s, u64 mCount, u64 mSize)
{
    u64 aligned = align8(mCount * mSize);
    u64 padded = aligned + sizeof(RBNode<FreeListNode>);

    RBNode<FreeListNode>* pLastFit = nullptr;
    {
        RBNode<FreeListNode>* node = s->tree.pRoot;
        while (node)
        {
            if (aligned <= node->data.size)
            {
                pLastFit = node;
                node = node->left;
            }
            else node = node->right;
        }
    }

    /* insert new block when empty */
    if (!pLastFit)
        pLastFit = FreeListNewBlockToTheEnd(
            s, utils::max(SIZE_1M, padded) - sizeof(FreeListBlock)
        );

    FreeListBlock* pBlock = FreeListFindBlock(s, pLastFit);

    u64 savedSize = pLastFit->data.size;
    auto* pEdge = (u8*)pBlock + pBlock->size;
    auto* splitNode = (RBNode<FreeListNode>*)((u8*)pLastFit + padded);

    RBRemove(&s->tree, pLastFit);

    /*COUT("edge: %lu, pLastFit: %lu, edge - last: %lu\n", (u64)pEdge, (u64)pLastFit, (u64)pEdge - (u64)pLastFit);*/

    /* split node only if there is some free space left */
    if (pLastFit->data.size > aligned && (u8*)splitNode < ((u8*)pEdge - sizeof(RBNode<FreeListNode>)))
    {
        pLastFit->data.next = splitNode;
        pLastFit->data.size = aligned;

        /*COUT("savedSize - padded: %lu\n", savedSize - padded);*/
        splitNode->data.size = savedSize - padded;
        splitNode->data.bFreed = true;
        splitNode->data.next = nullptr;
        splitNode->data.prev = pLastFit;

        RBInsert(&s->tree, splitNode, true);
    }

    pLastFit->data.bFreed = false;
    return pLastFit->data.pData;
}

inline void
FreeListMergeNext(FreeList* s, RBNode<FreeListNode>* pNode)
{
    auto* next = pNode->data.next;

    assert(pNode == next->data.prev);

    next->data.bFreed = false;
    RBRemove(&s->tree, next);

    auto* nextNext = next->data.next;
    if (nextNext) nextNext->data.prev = pNode;

    pNode->data.size += (next->data.size + sizeof(RBNode<FreeListNode>));
    pNode->data.next = nextNext;
}

inline void
FreeListMergePrev(FreeList* s, RBNode<FreeListNode>* pNode)
{
    auto* next = pNode->data.next;
    auto* prev = pNode->data.prev;

    if (prev->data.next != pNode) return;

    assert(prev->data.next == pNode);

    pNode->data.bFreed = false;
    RBRemove(&s->tree, pNode);

    if (next) next->data.prev = prev;
    prev->data.next = next;

    prev->data.size += (pNode->data.size + sizeof(RBNode<FreeListNode>));
}

inline void*
FreeListRealloc(FreeList* s, void* p, u64 mCount, u64 mSize)
{
    auto* pNode = (RBNode<FreeListNode>*)((u8*)p - sizeof(RBNode<FreeListNode>));

    /* TODO: bump on realloc when next free and fits */

    // auto* next = pNode->data.next;
    // if (next && next->data.bFreed && ((pNode->data.size + (next->data.size + sizeof(RBNode<FreeListNode>))) >= mCount * mSize))
    // {
    //     FreeListMergeNext(s, pNode);
    //     return p;
    // }

    /*COUT("size: %lu, mul: %lu\n", pNode->data.size, mCount * mSize);*/
    auto* r = FreeListAlloc(s, mCount, mSize);
    memcpy(r, p, pNode->data.size);

    FreeListFree(s, p);

    return r;
}

inline void
FreeListFree(FreeList* s, void* p)
{
    auto* pNode = (RBNode<FreeListNode>*)((u8*)p - sizeof(RBNode<FreeListNode>));
    memset(pNode->data.pData, 0, sizeof(pNode->data.size));
    pNode->data.bFreed = true;

    RBInsert(&s->tree, pNode, true);

    /* try to merge adjecent free nodes */

    auto* next = pNode->data.next;
    if (next && next->data.bFreed)
        FreeListMergeNext(s, pNode);

    auto* prev = pNode->data.prev;
    if (prev && prev->data.bFreed)
        FreeListMergePrev(s, pNode);
}

inline void
FreeListFreeAll(FreeList* s)
{
    ADT_FREELIST_FOREACH_SAFE(s, it, tmp)
        ::free(it);
}

inline FreeListBlock*
FreeListFindBlock(FreeList* s, RBNode<FreeListNode>* p)
{
    FreeListBlock* pBlock = nullptr;
    {
        ADT_FREELIST_FOREACH(s, pB)
            if ((u8*)p > (u8*)pB && ((u8*)pB + pB->size) > (u8*)p)
            {
                pBlock = pB;
                break;
            }
    }
    assert(pBlock && "can't find node's block (bad pointer / memory corruption)");

    return pBlock;
}

/* allocate new block and insert into the tree */
inline FreeListBlock*
FreeListNewBlock(FreeList *s, u64 size)
{
    auto* r = (FreeListBlock*)::calloc(1, size + sizeof(FreeListBlock));
    u64 blockSize = size - sizeof(FreeListBlock);
    r->size = blockSize;
    r->pNext = nullptr;

    auto* firstNode = (RBNode<FreeListNode>*)(r->pMem);

    firstNode->data.size = blockSize - sizeof(RBNode<FreeListNode>);
    firstNode->data.bFreed = true;
    firstNode->data.next = nullptr;
    firstNode->data.prev = nullptr;
    RBInsert(&s->tree, firstNode, true);

    return r;
}

inline RBNode<FreeListNode>*
FreeListNewBlockToTheEnd(FreeList* s, u64 size)
{
    /* insert new block */
    FreeListBlock** ppBlock = &s->pBlocks;
    while (*ppBlock) ppBlock = &(*ppBlock)->pNext;

    *ppBlock = FreeListNewBlock(s, size);
    return (RBNode<FreeListNode>*)((*ppBlock)->pMem);
}

inline const AllocatorInterface __FreeListVTable {
    .alloc = (decltype(AllocatorInterface::alloc))FreeListAlloc,
    .realloc = (decltype(AllocatorInterface::realloc))FreeListRealloc,
    .free = (decltype(AllocatorInterface::free))FreeListFree,
};

inline
FreeList::FreeList(u64 preallocBlock)
    : base {&__FreeListVTable}
{
    this->pBlocks = FreeListNewBlock(this, align8(preallocBlock));
}

} /* namespace adt */
