#include "adt/Pool.hh"

template<typename ALLOC_T, adt::u32 CAP>
struct AllocatorPool
{
    adt::Pool<ALLOC_T, CAP> pool {};

    /* */

    AllocatorPool() = default;
    AllocatorPool(adt::INIT_FLAG eFlag) : pool(eFlag) {}

    /* */

    template<typename ...ARGS>
    [[nodiscard]] ALLOC_T*
    get(ARGS&&... args)
    {
        adt::u32 idx = pool.emplace(std::forward<ARGS>(args)...);
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
