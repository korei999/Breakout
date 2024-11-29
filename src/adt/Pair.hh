#pragma once

#include "print.hh"

namespace adt
{

template<typename A, typename B>
struct Pair
{
    A x {};
    B y {};
};

template<typename A, typename B>
constexpr bool
operator==(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return l.x == r.x && l.y == r.y;
}

template<typename A, typename B>
constexpr bool
operator!=(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return !(l == r);
}

template<typename A, typename B>
constexpr bool
operator<(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return l.x < r.x && l.y < r.y;
}

template<typename A, typename B>
constexpr bool
operator>(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return l.x > r.x && l.y > r.y;
}

template<typename A, typename B>
constexpr bool
operator<=(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return l.x <= r.x && l.y <= r.y;
}

template<typename A, typename B>
constexpr bool
operator>=(const Pair<A, B>& l, const Pair<A, B>& r)
{
    return l.x >= r.x && l.y >= r.y;
}

namespace print
{

template<typename A, typename B>
inline u32
formatToContext(Context ctx, [[maybe_unused]] FormatArgs fmtArgs, const Pair<A, B>& x)
{
    ctx.fmt = "[{}, {}]";
    ctx.fmtIdx = 0;
    return printArgs(ctx, x.x, x.y);
}

} /* namespace print */

} /* namespace adt */
