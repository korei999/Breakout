#pragma once

#include "types.hh"

#include <cassert>

namespace adt
{

template<typename T>
struct TwoDSpan
{
    T* m_pData {};
    u32 m_width {};
    u32 m_height {};

    /* */

    constexpr TwoDSpan() = default;
    constexpr TwoDSpan(T* pData, u32 width, u32 height)
        : m_pData(pData), m_width(width), m_height(height) {}

    /* */

    constexpr T& operator[](u32 x, u32 y) { return at(x, y); }
    constexpr const T& operator[](u32 x, u32 y) const { return at(x, y); }

    constexpr T* data() { return m_pData; }
    constexpr const T* data() const { return m_pData; }

    constexpr u32 getWidth() const { return m_width; }
    constexpr u32 getHeight() const { return m_height; }

    /* */

private:
    constexpr T&
    at(u32 x, u32 y)
    {
        u32 idx = y*m_width + x;
        assert(x < m_width && y < m_height && idx < m_width*m_height && "[TwoDSpan]: out of range");
        return m_pData[idx];
    }
};

} /* namespace adt */
