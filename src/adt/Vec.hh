#pragma once

#include "Allocator.hh"
#include "utils.hh"

#include <assert.h>

namespace adt
{

#define ADT_ARRAY_FOREACH_I(A, I) for (u32 I = 0; I < (A)->size; I++)
#define ADT_ARRAY_FOREACH_I_REV(A, I) for (long I = (A)->size - 1; I >= 0 ; I--)

/* Dynamic array (aka Vector) */
template<typename T>
struct Vec
{
    Allocator* pAlloc = nullptr;
    T* pData = nullptr;
    u32 size = 0;
    u32 cap = 0;

    Vec() = default;
    Vec(Allocator* pA, u32 capacity = SIZE_MIN)
        : pAlloc {pA},
          pData {(T*)alloc(pA, capacity, sizeof(T))},
          size {0},
          cap {capacity} {}

    T& operator[](u32 i) { assert(i < size && "out of range access"); return pData[i]; }
    const T& operator[](u32 i) const { assert(i < size && "out of range access"); return pData[i]; }

    struct It
    {
        T* p;

        It(T* pFirst) : p{pFirst} {}

        T& operator*() { return *p; }
        T* operator->() { return p; }

        It operator++() { p++; return *this; }
        It operator++(int) { T* tmp = p++; return tmp; }

        It operator--() { p--; return *this; }
        It operator--(int) { T* tmp = p--; return tmp; }

        friend constexpr bool operator==(const It& l, const It& r) { return l.p == r.p; }
        friend constexpr bool operator!=(const It& l, const It& r) { return l.p != r.p; }
    };

    It begin() { return {&this->pData[0]}; }
    It end() { return {&this->pData[this->size]}; }
    It rbegin() { return {&this->pData[this->size - 1]}; }
    It rend() { return {this->pData - 1}; }

    const It begin() const { return begin(); }
    const It end() const { return end(); }
    const It rbegin() const { return rbegin(); }
    const It rend() const { return rend(); }
};

template<typename T> inline void VecPush(Vec<T>* s, const T& data);
template<typename T> inline void VecGrow(Vec<T>* s, u32 size);
template<typename T> inline T& VecLast(Vec<T>* s);
template<typename T> inline T& VecFirst(Vec<T>* s);
template<typename T> inline T* VecPop(Vec<T>* s);
template<typename T> inline void VecSetSize(Vec<T>* s, u32 size);
template<typename T> inline void VecSwapWithLast(Vec<T>* s, u32 i);
template<typename T> inline void VecPopAsLast(Vec<T>* s, u32 i);
template<typename T> inline void VecDestroy(Vec<T>* s);

template<typename T>
inline void
VecPush(Vec<T>* s, const T& data)
{
    assert(s->cap > 0 && "uninitialized array push");

    if (s->size >= s->cap)
        VecGrow(s, s->cap * 2);

    s->pData[s->size++] = data;
}

template<typename T>
inline void
VecGrow(Vec<T>* s, u32 size)
{
    s->cap = size;
    s->pData = (T*)realloc(s->pAlloc, s->pData, sizeof(T), size);
}

template<typename T>
inline T&
VecLast(Vec<T>* s)
{
    return s->pData[s->size - 1];
}

template<typename T>
inline T&
VecFirst(Vec<T>* s)
{
    return s->pData[0];
}

template<typename T>
inline T*
VecPop(Vec<T>* s)
{
    assert(s->size > 0 && "popping from the empty array");
    return &s->pData[--s->size];
}

template<typename T>
inline void
VecSetSize(Vec<T>* s, u32 size)
{
    if (s->size < size)
        VecGrow(s, size);

    s->size = size;
}

template<typename T>
inline void
VecSwapWithLast(Vec<T>* s, u32 i)
{
    utils::swap(&s->pData[i], &s->pData[s->size - 1]);
}

template<typename T>
inline void
VecPopAsLast(Vec<T>* s, u32 i)
{
    s->pData[i] = s->pData[--s->size];
}

template<typename T>
inline void
VecDestroy(Vec<T>* s)
{
    free(s->pAlloc, s->pData);
}

} /* namespace adt */
