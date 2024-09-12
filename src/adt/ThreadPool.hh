#pragma once

#include <atomic>
#include <threads.h>

#include "Queue.hh"

#ifdef __linux__
    #include <sys/sysinfo.h>

    #define getLogicalCoresCount() get_nprocs()
#elif _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>

inline DWORD
getLogicalCoresCountWIN32()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

    #define getLogicalCoresCount() getLogicalCoresCountWIN32()

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
#else
    #define getLogicalCoresCount() 4
#endif

namespace adt
{

struct ThreadPoolTask
{
    thrd_start_t pfn;
    void* pArgs;
};

struct ThreadPool
{
    Queue<ThreadPoolTask> qTasks {};
    thrd_t* pThreads = nullptr;
    u32 threadCount = 0;
    cnd_t cndQ, cndWait;
    mtx_t mtxQ, mtxWait;
    std::atomic<int> activeTaskCount {};
    bool bDone = false;

    ThreadPool() = default;
    ThreadPool(Allocator* p, u32 _threadCount);
    ThreadPool(Allocator* p) : ThreadPool(p, getLogicalCoresCount()) {}
};

inline void ThreadPoolStart(ThreadPool* s);
inline bool ThreadPoolBusy(ThreadPool* s);
inline void ThreadPoolSubmit(ThreadPool* s, ThreadPoolTask task);
inline void ThreadPoolSubmit(ThreadPool* s, thrd_start_t pfnTask, void* pArgs);
inline void ThreadPoolWait(ThreadPool* s);

inline
ThreadPool::ThreadPool(Allocator* p, u32 _threadCount)
    : qTasks (p, _threadCount), threadCount (_threadCount), activeTaskCount (0), bDone (false)
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
        ThreadPoolTask task;
        {
            mtx_lock(&s->mtxQ);

            while (QueueEmpty(&s->qTasks) && !s->bDone)
                cnd_wait(&s->cndQ, &s->mtxQ);

            if (s->bDone)
            {
                mtx_unlock(&s->mtxQ);
                return thrd_success;
            }

            task = *QueuePopFront(&s->qTasks);
            s->activeTaskCount++; /* increment before unlocking mtxQ to avoid 0 tasks and 0 q possibility */

            mtx_unlock(&s->mtxQ);
        }

        task.pfn(task.pArgs);
        s->activeTaskCount--;

        if (!ThreadPoolBusy(s))
            cnd_signal(&s->cndWait);
    }

    return thrd_success;
}

inline void
ThreadPoolStart(ThreadPool* s)
{
    for (size_t i = 0; i < s->threadCount; i++)
        thrd_create(&s->pThreads[i], __ThreadPoolLoop, s);
}

inline bool
ThreadPoolBusy(ThreadPool* s)
{
    mtx_lock(&s->mtxQ);
    bool ret = !QueueEmpty(&s->qTasks);
    mtx_unlock(&s->mtxQ);

    return ret || s->activeTaskCount > 0;
}

inline void
ThreadPoolSubmit(ThreadPool* s, ThreadPoolTask task)
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
    for (u32 i = 0; i < s->threadCount; i++)
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
