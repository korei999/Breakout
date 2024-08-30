#pragma once

#include "Allocator.hh"

#include <assert.h>

namespace adt
{

#define ADT_QUEUE_FOREACH_I(Q, I) for (int I = (Q)->first, __t = 0; __t < (Q)->size; I = QueueNextI(Q, I), __t++)
#define ADT_QUEUE_FOREACH_I_REV(Q, I) for (int I = QueueLastI(Q), __t = 0; __t < (Q)->size; I = QueuePrevI(Q, I), __t++)

template<typename T> struct Queue;

template<typename T> inline bool QueueEmpty(Queue<T>* s);
template<typename T> inline T* QueuePushBack(Queue<T>* s, const T& val);
template<typename T> inline void QueueResize(Queue<T>* s, u32 size);
template<typename T> inline T* QueuePopFront(Queue<T>* s);
template<typename T> inline T* QueuePopBack(Queue<T>* s);
template<typename T> inline void QueueDestroy(Queue<T>*s);

template<typename T> inline int QueueNextI(Queue<T>*s, int i) { return (i + 1) >= s->cap ? 0 : (i + 1); }
template<typename T> inline int QueuePrevI(Queue<T>* s, int i) { return (i - 1) < 0 ? s->cap - 1 : (i - 1); }
template<typename T> inline int QueueFirstI(Queue<T>* s) { return QueueEmpty(s) ? -1 : s->first; }
template<typename T> inline int QueueLastI(Queue<T>* s) { return QueueEmpty(s) ? 0 : s->last - 1; }

template<typename T>
struct Queue
{
    Allocator* pAlloc {};
    T* pData {};
    int size {};
    int cap {};
    int first {};
    int last {};

    Queue() = default;
    Queue(Allocator* p) : Queue(p, SIZE_MIN) {}
    Queue(Allocator* p, u32 prealloc)
        : pAlloc (p),
          pData {(T*)alloc(pAlloc, prealloc, sizeof(T))},
          cap (prealloc) {}

    T& operator[](int i) { return pData[i]; }
    const T& operator[](int i) const { return pData[i]; }

    struct It
    {
        Queue* s = nullptr;
        int i = 0;
        int counter = 0;

        It(Queue* _s, int _i, int _counter) : s{_s}, i{_i}, counter{_counter} {}

        T& operator*() const { return s->pData[i]; }
        T* operator->() const { return &s->pData[i]; }

        It
        operator++()
        {
            i = QueueNextI(s, i);
            counter++;
            return {s, i, counter};
        }

        It operator++(int) { It tmp = *this; *this++; return tmp; }

        It
        operator--()
        {
            i = QueuePrevI(s, i);
            counter++;
            return {s, i, counter};
        }

        It operator--(int) { It tmp = *this; *this--; return tmp; }

        friend bool operator==(const It& l, const It& r) { return l.counter == r.counter; }
        friend bool operator!=(const It& l, const It& r) { return l.counter != r.counter; }
    };

    It begin() { return {this, QueueFirstI(this), 0}; }
    It end() { return {this, {}, this->size}; }
    It rbegin() { return {this, QueueLastI(this), 0}; }
    It rend() { return {this, {}, this->size}; }

    const It begin() const { return {this, QueueFirstI(this), 0}; }
    const It end() const { return {this, {}, this->size}; }
    const It rbegin() const { return {this, QueueLastI(this), 0}; }
    const It rend() const { return {this, {}, this->size}; }
};

template<typename T>
inline bool
QueueEmpty(Queue<T>* s)
{
    return s->size == 0;
}

template<typename T>
inline T*
QueuePushBack(Queue<T>* s, const T& val)
{
    if (s->size >= s->cap)
        QueueResize(s, s->cap * 2);
    
    int i = s->last;
    int ni = QueueNextI(s, i);
    s->pData[i] = val;
    s->last = ni;
    s->size++;

    return &s->pData[i];
}

template<typename T>
inline void
QueueResize(Queue<T>* s, u32 size)
{
    auto nQ = Queue<T>(s->pAlloc, size);

    for (auto& e : *s)
        QueuePushBack(&nQ, e);

    QueueDestroy(s);
    *s = nQ;
}


template<typename T>
inline T*
QueuePopFront(Queue<T>* s)
{
    assert(s->size > 0);

    T* ret = &s->pData[s->first];
    s->first = QueueNextI(s, s->first);
    s->size--;

    return ret;
}

template<typename T>
inline T*
QueuePopBack(Queue<T>* s)
{
    assert(s->size > 0);

    T* ret = &s->pData[QueueLastI(s)];
    s->last = QueuePrevI(s, QueueLastI(s));
    s->size--;

    return ret;
}

template<typename T>
inline void
QueueDestroy(Queue<T>*s)
{
    free(s->pAlloc, s->pData);
}

} /* namespace adt */
