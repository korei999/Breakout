#include "adt/Pool.hh"

template<typename ALLOC_T, adt::u32 CAP>
struct AllocatorPool
{
    adt::Pool<ALLOC_T, CAP> pool {};

    [[nodiscard]] ALLOC_T*
    get(adt::u32 size)
    {
        adt::u32 idx = pool.getHandle(size);
        return &pool[idx];
    }

    void
    giveBack(ALLOC_T* pAlloc)
    {
        pAlloc->freeAll();
        pool.giveBack(pool.idx(pAlloc));
    }

    void
    freeAll()
    {
        for (auto& alloc : pool)
            alloc.freeAll();
    }
};
