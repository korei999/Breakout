#pragma once

#include "utils.hh"
#include "types.hh"

#include <assert.h>

namespace adt
{

/* statically sized array */
template<typename T, u32 SIZE>
struct Arr
{
    T pData[SIZE] {};
    u32 size {};

    T& operator[](u32 i)             { assert(i < SIZE && "[Arr]: out of range access"); return pData[i]; }
    const T& operator[](u32 i) const { assert(i < SIZE && "[Arr]: out of range access"); return pData[i]; }

    struct It
    {
        T* s;

        It(T* pFirst) : s{pFirst} {}

        T& operator*() { return *s; }
        T* operator->() { return s; }

        It operator++() { s++; return *this; }
        It operator++(int) { T* tmp = s++; return tmp; }

        It operator--() { s--; return *this; }
        It operator--(int) { T* tmp = s--; return tmp; }

        friend constexpr bool operator==(const It& l, const It& r) { return l.s == r.s; }
        friend constexpr bool operator!=(const It& l, const It& r) { return l.s != r.s; }
    };

    It begin() { return {&this->pData[0]}; }
    It end() { return {&this->pData[this->size]}; }
    It rbegin() { return {&this->pData[this->size - 1]}; }
    It rend() { return {this->pData - 1}; }

    const It begin() const { return {&this->pData[0]}; }
    const It end() const { return {&this->pData[this->size]}; }
    const It rbegin() const { return {&this->pData[this->size - 1]}; }
    const It rend() const { return {this->pData - 1}; }
};

template<typename T, u32 SIZE>
inline u32
ArrPush(Arr<T, SIZE>* s, const T& x)
{
    assert(SIZE > 0 && "[Arr]: ininitialized push");
    assert(s->size < SIZE && "[Arr]: pushing over capacity");

    s->pData[s->size++] = x;
    return s->size - 1;
}

template<typename T, u32 SIZE>
inline u64
ArrCap(Arr<T, SIZE>* s)
{
    return utils::size(s->pData);
}

template<typename T, u32 SIZE>
inline u32
ArrSize(Arr<T, SIZE>* s)
{
    return s->size;
}

template<typename T, u32 SIZE>
inline void
ArrSetSize(Arr<T, SIZE>* s, u32 newSize)
{
    assert(newSize <= SIZE && "[Arr]: cannot enlarge static array");
    s->size = newSize;
}

} /* namespace adt */
