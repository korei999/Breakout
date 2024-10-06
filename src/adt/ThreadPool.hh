#pragma once

#include "Queue.hh"
#include "defer.hh"

#include <threads.h>
#include <stdatomic.h>

namespace adt
{

#ifdef __linux__
    #include <sys/sysinfo.h>

    #define ADT_GET_NCORES() get_nprocs()
#elif _WIN32
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
    #ifdef near
        #undef near
    #endif
    #ifdef far
        #undef far
    #endif
    #include <sysinfoapi.h>

inline DWORD
getLogicalCoresCountWIN32()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

    #define ADT_GET_NCORES() getLogicalCoresCountWIN32()
#else
    #define ADT_GET_NCORES() 4
#endif

struct ThreadTask
{
    thrd_start_t pfn;
    void* pArgs;
};

struct ThreadPool
{
    Queue<ThreadTask> qTasks {};
    thrd_t* pThreads = nullptr;
    u32 nThreads = 0;
    cnd_t cndQ, cndWait;
    mtx_t mtxQ, mtxWait;
    _Atomic(int) nActiveTasksAtomic;
    bool bDone = false;

    ThreadPool() = default;
    ThreadPool(Allocator* p, u32 _threadCount = ADT_GET_NCORES());
};

inline void ThreadPoolStart(ThreadPool* s);
inline bool ThreadPoolBusy(ThreadPool* s);
inline void ThreadPoolSubmit(ThreadPool* s, ThreadTask task);
inline void ThreadPoolSubmit(ThreadPool* s, thrd_start_t pfnTask, void* pArgs);
inline void ThreadPoolWait(ThreadPool* s);

inline
ThreadPool::ThreadPool(Allocator* p, u32 _threadCount)
    : qTasks(p, _threadCount), nThreads(_threadCount), nActiveTasksAtomic(0), bDone(false)
{
    /*QueueResize(&qTasks, _threadCount);*/
    pThreads = (thrd_t*)alloc(p, _threadCount, sizeof(thrd_t));
    cnd_init(&cndQ);
    mtx_init(&mtxQ, mtx_plain);
    cnd_init(&cndWait);
    mtx_init(&mtxWait, mtx_plain);
}

inline int
__ThreadPoolLoop(void* p)
{
    auto* s = (ThreadPool*)p;

    while (!s->bDone)
    {
        ThreadTask task;
        {
            mtx_lock(&s->mtxQ);
            defer(mtx_unlock(&s->mtxQ));

            while (QueueEmpty(&s->qTasks) && !s->bDone)
                cnd_wait(&s->cndQ, &s->mtxQ);

            if (s->bDone) return thrd_success;

            task = *QueuePopFront(&s->qTasks);
            s->nActiveTasksAtomic++; /* increment before unlocking mtxQ to avoid 0 tasks and 0 q possibility */
        }

        task.pfn(task.pArgs);
        s->nActiveTasksAtomic--;

        if (!ThreadPoolBusy(s))
            cnd_signal(&s->cndWait);
    }

    return thrd_success;
}

inline void
ThreadPoolStart(ThreadPool* s)
{
    for (size_t i = 0; i < s->nThreads; i++)
        thrd_create(&s->pThreads[i], __ThreadPoolLoop, s);
}

inline bool
ThreadPoolBusy(ThreadPool* s)
{
    mtx_lock(&s->mtxQ);
    bool ret = !QueueEmpty(&s->qTasks);
    mtx_unlock(&s->mtxQ);

    return ret || s->nActiveTasksAtomic > 0;
}

inline void
ThreadPoolSubmit(ThreadPool* s, ThreadTask task)
{
    mtx_lock(&s->mtxQ);
    QueuePushBack(&s->qTasks, task);
    mtx_unlock(&s->mtxQ);

    cnd_signal(&s->cndQ);
}

inline void
ThreadPoolSubmit(ThreadPool* s, thrd_start_t pfnTask, void* pArgs)
{
    ThreadPoolSubmit(s, {pfnTask, pArgs});
}

inline void
ThreadPoolWait(ThreadPool* s)
{
    while (ThreadPoolBusy(s))
    {
        mtx_lock(&s->mtxWait);
        cnd_wait(&s->cndWait, &s->mtxWait);
        mtx_unlock(&s->mtxWait);
    }
}

inline void
__ThreadPoolStop(ThreadPool* s)
{
    s->bDone = true;
    cnd_broadcast(&s->cndQ);
    for (u32 i = 0; i < s->nThreads; i++)
        thrd_join(s->pThreads[i], nullptr);
}

inline void
ThreadPoolDestroy(ThreadPool* s)
{
    __ThreadPoolStop(s);

    free(s->qTasks.pAlloc, s->pThreads);
    QueueDestroy(&s->qTasks);
    cnd_destroy(&s->cndQ);
    mtx_destroy(&s->mtxQ);
    cnd_destroy(&s->cndWait);
    mtx_destroy(&s->mtxWait);
}

} /* namespace adt */
