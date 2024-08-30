#pragma once

#ifdef __linux__
    #include <time.h>
#elif _WIN32
    #include <sysinfoapi.h>
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
#endif

#include "types.hh"

#include <string.h>

namespace adt
{
namespace utils
{

template<typename T> constexpr void swap(T* l, T* r);
[[nodiscard]] constexpr auto& max(const auto& l, const auto& r);
[[nodiscard]] constexpr auto& min(const auto& l, const auto& r);
template<typename T> constexpr u64 size(const T& a);
template<typename T> constexpr bool odd(const T& a);
template<typename T> constexpr bool even(const T& a);
template<typename T> constexpr s64 compare(const T& l, const T& r);
inline f64 timeNowMS();
inline f64 timeNowS();
template<typename T> constexpr int partition(T a[], int l, int h);
template<typename T> constexpr void qSort(T a[], int l, int h);
template<typename T> constexpr void qSort(T* a);
template<typename T> constexpr void copy(T* pDest, T* pSrc, u64 size); /* memcpy with size * sizeof(T) */

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
constexpr u64
size(const T& a)
{
    return sizeof(a) / sizeof(a[0]);
}

template<typename T>
constexpr bool
odd(const T& a)
{
    return a & 1;
}

template<typename T>
constexpr bool
even(const T& a)
{
    return !odd(a);
}

/* negative is l < r, positive if l > r, 0 if l == r */
template<typename T>
constexpr s64
compare(const T& l, const T& r)
{
    return l - r;
}

inline f64
timeNowMS()
{
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_t micros = ts.tv_sec * 1000000000;
    micros += ts.tv_nsec;

    return micros / 1000000.0;

#elif _WIN32
    LARGE_INTEGER count, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);

    auto ret = (count.QuadPart * 1000000) / freq.QuadPart;
    return f64(ret) / 1000.0;
#endif
}

inline f64
timeNowS()
{
    return timeNowMS() / 1000.0;
}

template<typename T>
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
constexpr void
copy(T* pDest, T* pSrc, u64 size)
{
    memcpy(pDest, pSrc, size * sizeof(T));
}

} /* namespace utils */
} /* namespace adt */
