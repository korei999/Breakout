#pragma once

#ifdef __linux__
    #include <ctime>
    #include <unistd.h>
#elif _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN 1
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #undef near
    #undef far
    #undef NEAR
    #undef FAR
    #undef min
    #undef max
    #undef MIN
    #undef MAX
    #include <sysinfoapi.h>
#endif

#include "types.hh"

#include <cstring>

namespace adt
{
namespace utils
{

template<typename T>
constexpr void
swap(T* l, T* r)
{
    auto tmp = *l;
    *l = *r;
    *r = tmp;
}

[[nodiscard]]
constexpr auto&
max(const auto& l, const auto& r)
{
    return l > r ? l : r;
}

[[nodiscard]]
constexpr auto&
min(const auto& l, const auto& r)
{
    return l < r ? l : r;
}

template<typename T>
[[nodiscard]]
constexpr u64
size(const T& a)
{
    return sizeof(a) / sizeof(a[0]);
}

template<typename T>
[[nodiscard]]
constexpr bool
odd(const T& a)
{
    return a & 1;
}

template<typename T>
[[nodiscard]]
constexpr bool
even(const T& a)
{
    return !odd(a);
}

/* negative is l < r, positive if l > r, 0 if l == r */
template<typename T>
[[nodiscard]]
constexpr s64
compare(const T& l, const T& r)
{
    return l - r;
}

[[nodiscard]]
inline long
timeNowUS()
{
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_t micros = ts.tv_sec * 1000000000;
    micros += ts.tv_nsec;

    return micros;

#elif _WIN32
    LARGE_INTEGER count, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);

    return (count.QuadPart * 1000000000) / freq.QuadPart;
#endif
}

[[nodiscard]]
inline f64
timeNowMS()
{
#ifdef __linux__
    return timeNowUS() / 1000000.0;

#elif _WIN32
    LARGE_INTEGER count, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);

    auto ret = (count.QuadPart * 1000000) / freq.QuadPart;
    return f64(ret) / 1000.0;
#endif
}

[[nodiscard]]
inline f64
timeNowS()
{
    return timeNowMS() / 1000.0;
}

inline void
sleepMS(f64 ms)
{
#ifdef __linux__
    usleep(ms * 1000.0);
#elif _WIN32
    Sleep(ms);
#endif
}

template<typename T>
[[nodiscard]]
constexpr int
partition(T a[], int l, int h)
{
    int p = h, firstHigh = l;

    for (int i = l; i < h; i++)
        if (a[i] < a[p])
        {
            swap(&a[i], &a[firstHigh]);
            firstHigh++;
        }

    swap(&a[p], &a[firstHigh]);

    return firstHigh;
}

template<typename T>
constexpr void
qSort(T a[], int l, int h)
{
    int p;

    if (l < h)
    {
        p = partition(a, l, h);
        qSort(a, l, p - 1);
        qSort(a, p + 1, h);
    }
}

template<typename T>
constexpr void
qSort(T* a)
{
    qSort(a->pData, 0, a->size - 1);
}

template<typename T>
inline void
copy(T* pDest, T* pSrc, u64 size)
{
    memcpy(pDest, pSrc, size * sizeof(T));
}

template<typename T>
inline void
fill(T* pData, T x, u64 size)
{
    for (u64 i = 0; i < size; ++i)
        pData[i] = x;
}

template<typename T>
[[nodiscard]]
constexpr T
clamp(const T& x, const T& _min, const T& _max)
{
    return max(_min, min(_max, x));
}

template<template<typename> typename CON, typename T>
inline T&
searchMax(CON<T>& s)
{
    auto _max = s.begin();
    for (auto it = ++s.begin(); it != s.end(); ++it)
        if (*it > *_max) _max = it;

    return *_max;
}

template<template<typename> typename CON, typename T>
inline T&
searchMin(CON<T>& s)
{
    auto _min = s.begin();
    for (auto it = ++s.begin(); it != s.end(); ++it)
        if (*it < *_min) _min = it;

    return *_min;
}

} /* namespace utils */
} /* namespace adt */
