#pragma once

#include <adt/types.hh>
#include <assert.h>

#define ADT_MATH_COLOR4(hex)                                                                                           \
    {                                                                                                                  \
        ((hex >> 24) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

#define ADT_MATH_COLOR3(hex)                                                                                           \
    {                                                                                                                  \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

namespace adt
{
namespace math
{

constexpr f64 PI = 3.14159265358979323846;
constexpr f32 PI_F = f32(PI);
constexpr f64 EPS = 0.000001;
constexpr f32 EPS_F = 0.00001f;

constexpr f64 toDeg(f64 x) { return x * 180.0 / PI; }
constexpr f64 toRad(f64 x) { return x * PI / 180.0; }
constexpr f32 toDeg(f32 x) { return x * 180.0f / PI_F; }
constexpr f32 toRad(f32 x) { return x * PI_F / 180.0f; }

constexpr f64 toRad(long x) { return toRad(f64(x)); }
constexpr f64 toDeg(long x) { return toDeg(f64(x)); }
constexpr f32 toRad(int x) { return toRad(f32(x)); }
constexpr f32 toDeg(int x) { return toDeg(f32(x)); }

constexpr bool rEq(f64 l, f64 r) { return l >= r - EPS && l <= r + EPS; } /* roughly equals */
constexpr bool rEq(f32 l, f32 r) { return l >= r - EPS_F && l <= r + EPS_F; } /* roughly equals */

template<typename T> constexpr T sq(const T& x) { return x * x; }

union V2
{
    f32 e[2];
    struct {
        f32 x, y;
    };
    struct {
        f32 u, v;
    };
};

union V3
{
    f32 e[3];
    struct {
        V2 xy;
        f32 _v2pad;
    };
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
};

union V4
{
    f32 e[4];
    struct {
        V3 xyz;
        f32 __v3pad;
    };
    struct {
        V2 xy;
        V2 zw;
    };
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
};

union M2
{
    f32 d[4];
    f32 e[2][2];
    V2 v[2];
};

union M3
{
    f32 d[9];
    f32 e[3][3];
    V3 v[3];
};

union M4
{
    f32 d[16];
    f32 e[4][4];
    V4 v[4];
};

inline M3
M4ToM3(const M4& s)
{
    auto& e = s.e;
    return {
        e[0][0], e[0][1], e[0][2],
        e[1][0], e[1][1], e[1][2],
        e[2][0], e[2][1], e[2][2]
    };
}

struct M3BindToLetters
{
    const f32& a, & b, & c;
    const f32& d, & e, & f;
    const f32& g, & h, & i;

    M3BindToLetters() = delete;
    M3BindToLetters(const M3& m)
        : a(m.e[0][0]), b(m.e[0][1]), c(m.e[0][2]),
          d(m.e[1][0]), e(m.e[1][1]), f(m.e[1][2]),
          g(m.e[2][0]), h(m.e[2][1]), i(m.e[2][2]) {}
};

constexpr V2
operator+(const V2& l, const V2& r)
{
    return {
        .x = l.x + r.x,
        .y = l.y + r.y
    };
}

constexpr V2
operator-(const V2& l, const V2& r)
{
    return {
        .x = l.x - r.x,
        .y = l.y - r.y
    };
}

constexpr V2
operator*(const V2& v, f32 s)
{
    return {
        .x = v.x * s,
        .y = v.x * s
    };
}

constexpr V2
operator/(const V2& v, f32 s)
{
    return {
        .x = v.x / s,
        .y = v.x / s
    };
}

constexpr V2&
operator+=(V2& l, const V2& r)
{
    return l = l + r;
}

constexpr V2&
operator-=(V2& l, const V2& r)
{
    return l = l - r;
}

constexpr V2&
operator*=(V2& l, f32 r)
{
    return l = l * r;
}

constexpr V2&
operator/=(V2& l, f32 r)
{
    return l = l / r;
}

constexpr V3
operator+(const V3& l, const V3& r)
{
    return {
        .x = l.x + r.x,
        .y = l.y + r.y,
        .z = l.z + r.z
    };
}

constexpr V3
operator-(const V3& l, const V3& r)
{
    return {
        .x = l.x - r.x,
        .y = l.y - r.y,
        .z = l.z - r.z
    };
}

constexpr V3
operator*(const V3& v, f32 s)
{
    return {
        .x = v.x * s,
        .y = v.y * s,
        .z = v.z * s
    };
}

constexpr V3
operator/(const V3& v, f32 s)
{
    return {
        .x = v.x / s,
        .y = v.y / s,
        .z = v.z / s
    };
}

constexpr V3&
operator+=(V3& l, const V3& r)
{
    return l = l + r;
}

constexpr V3&
operator-=(V3& l, const V3& r)
{
    return l = l - r;
}

constexpr V3&
operator*=(V3& v, f32 s)
{
    return v = v * s;
}

constexpr V3&
operator/=(V3& v, f32 s)
{
    return v = v / s;
}

constexpr V4
operator+(const V4& l, const V4& r)
{
    return {
        .x = l.x + r.x,
        .y = l.y + r.y,
        .z = l.z + r.z,
        .w = l.w + r.w
    };
}

constexpr V4
operator-(const V4& l, const V4& r)
{
    return {
        .x = l.x - r.x,
        .y = l.y - r.y,
        .z = l.z - r.z,
        .w = l.w - r.w
    };
}

constexpr V4
operator*(const V4& l, f32 r)
{
    return {
        .x = l.x * r,
        .y = l.y * r,
        .z = l.z * r,
        .w = l.w * r
    };
}

constexpr V4
operator/(const V4& l, f32 r)
{
    return {
        .x = l.x / r,
        .y = l.y / r,
        .z = l.z / r,
        .w = l.w / r
    };
}

constexpr V4&
operator+=(V4& l, const V4& r)
{
    return l = l + r;
}

constexpr V4&
operator-=(V4& l, const V4& r)
{
    return l = l - r;
}

constexpr V4&
operator*=(V4& l, f32 r)
{
    return l = l * r;
}

constexpr V4&
operator/=(V4& l, f32 r)
{
    return l = l / r;
}

constexpr M2
M2Iden()
{
    return {
        1, 0,
        0, 1,
    };
}

constexpr M3
M3Iden()
{
    return {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };
}

constexpr M4
M4Iden()
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

inline f32
M2Det(const M2& s)
{
    auto& d = s.d;
    return d[0]*d[3] - d[1]*d[2];
}

inline f32
M3Det(const M3& s)
{
    auto& e = s.e;
    return (
        e[0][0] * (e[1][1] * e[2][2] - e[2][1] * e[1][2]) -
        e[0][1] * (e[1][0] * e[2][2] - e[1][2] * e[2][0]) +
        e[0][2] * (e[1][0] * e[2][1] - e[1][1] * e[2][0])
    );
}

inline f32
M4Det(const M4& s)
{
    auto& e = s.e;
    return (
        e[0][3] * e[1][2] * e[2][1] * e[3][0] - e[0][2] * e[1][3] * e[2][1] * e[3][0] -
        e[0][3] * e[1][1] * e[2][2] * e[3][0] + e[0][1] * e[1][3] * e[2][2] * e[3][0] +
        e[0][2] * e[1][1] * e[2][3] * e[3][0] - e[0][1] * e[1][2] * e[2][3] * e[3][0] -
        e[0][3] * e[1][2] * e[2][0] * e[3][1] + e[0][2] * e[1][3] * e[2][0] * e[3][1] +
        e[0][3] * e[1][0] * e[2][2] * e[3][1] - e[0][0] * e[1][3] * e[2][2] * e[3][1] -
        e[0][2] * e[1][0] * e[2][3] * e[3][1] + e[0][0] * e[1][2] * e[2][3] * e[3][1] +
        e[0][3] * e[1][1] * e[2][0] * e[3][2] - e[0][1] * e[1][3] * e[2][0] * e[3][2] -
        e[0][3] * e[1][0] * e[2][1] * e[3][2] + e[0][0] * e[1][3] * e[2][1] * e[3][2] +
        e[0][1] * e[1][0] * e[2][3] * e[3][2] - e[0][0] * e[1][1] * e[2][3] * e[3][2] -
        e[0][2] * e[1][1] * e[2][0] * e[3][3] + e[0][1] * e[1][2] * e[2][0] * e[3][3] +
        e[0][2] * e[1][0] * e[2][1] * e[3][3] - e[0][0] * e[1][2] * e[2][1] * e[3][3] -
        e[0][1] * e[1][0] * e[2][2] * e[3][3] + e[0][0] * e[1][1] * e[2][2] * e[3][3]
    );
}

inline M3
M3Minors(const M3& s)
{
    const auto& e = s.e;
    return {.d {
            M2Det({.d {e[1][1], e[1][2], e[2][1], e[2][2]} }),
            M2Det({.d {e[1][0], e[1][2], e[2][0], e[2][2]} }),
            M2Det({.d {e[1][0], e[1][1], e[2][0], e[2][1]} }),

            M2Det({.d {e[0][1], e[0][2], e[2][1], e[2][2]} }),
            M2Det({.d {e[0][0], e[0][2], e[2][0], e[2][2]} }),
            M2Det({.d {e[0][0], e[0][1], e[2][0], e[2][1]} }),

            M2Det({.d {e[0][1], e[0][2], e[1][1], e[1][2]} }),
            M2Det({.d {e[0][0], e[0][2], e[1][0], e[1][2]} }),
            M2Det({.d {e[0][0], e[0][1], e[1][0], e[1][1]} })
        }
    };
}

inline M4
M4Minors(const M4& s)
{
    const auto& e = s.e;
    return {.d {
            M3Det({.d {e[1][1], e[1][2], e[1][3],    e[2][1], e[2][2], e[2][3],    e[3][1], e[3][2], e[3][3]} }),
            M3Det({.d {e[1][0], e[1][2], e[1][3],    e[2][0], e[2][2], e[2][3],    e[3][0], e[3][2], e[3][3]} }),
            M3Det({.d {e[1][0], e[1][1], e[1][3],    e[2][0], e[2][1], e[2][3],    e[3][0], e[3][1], e[3][3]} }),
            M3Det({.d {e[1][0], e[1][1], e[1][2],    e[2][0], e[2][1], e[2][2],    e[3][0], e[3][1], e[3][2]} }),

            M3Det({.d {e[0][1], e[0][2], e[0][3],    e[2][1], e[2][2], e[2][3],    e[3][1], e[3][2], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][2], e[0][3],    e[2][0], e[2][2], e[2][3],    e[3][0], e[3][2], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][3],    e[2][0], e[2][1], e[2][3],    e[3][0], e[3][1], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][2],    e[2][0], e[2][1], e[2][2],    e[3][0], e[3][1], e[3][2]} }),

            M3Det({.d {e[0][1], e[0][2], e[0][3],    e[1][1], e[1][2], e[1][3],    e[3][1], e[3][2], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][2], e[0][3],    e[1][0], e[1][2], e[1][3],    e[3][0], e[3][2], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][3],    e[1][0], e[1][1], e[1][3],    e[3][0], e[3][1], e[3][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][2],    e[1][0], e[1][1], e[1][2],    e[3][0], e[3][1], e[3][2]} }),

            M3Det({.d {e[0][1], e[0][2], e[0][3],    e[1][1], e[1][2], e[1][3],    e[2][1], e[2][2], e[2][3]} }),
            M3Det({.d {e[0][0], e[0][2], e[0][3],    e[1][0], e[1][2], e[1][3],    e[2][0], e[2][2], e[2][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][3],    e[1][0], e[1][1], e[1][3],    e[2][0], e[2][1], e[2][3]} }),
            M3Det({.d {e[0][0], e[0][1], e[0][2],    e[1][0], e[1][1], e[1][2],    e[2][0], e[2][1], e[2][2]} })
        }
    };
}

inline M3
M3Cofactors(const M3& s)
{
    M3 minors = M3Minors(s);
    auto& e = minors.e;
    return {
        e[0][0] *  1, e[0][1] * -1, e[0][2] *  1,
        e[1][0] * -1, e[1][1] *  1, e[1][2] * -1,
        e[2][0] *  1, e[2][1] * -1, e[2][2] *  1
    };
}

inline M4
M4Cofactors(const M4& s)
{
    M4 minors = M4Minors(s);
    auto& e = minors.e;
    return {
        e[0][0] * +1, e[0][1] * -1, e[0][2] * +1, e[0][3] * -1,
        e[1][0] * -1, e[1][1] * +1, e[1][2] * -1, e[1][3] * +1,
        e[2][0] * +1, e[2][1] * -1, e[2][2] * +1, e[2][3] * -1,
        e[3][0] * -1, e[3][1] * +1, e[3][2] * -1, e[3][3] * +1
    };
}

inline M3
M3Transpose(const M3& s)
{
    auto& e = s.e;
    return {
        e[0][0], e[1][0], e[2][0],
        e[0][1], e[1][1], e[2][1],
        e[0][2], e[1][2], e[2][2]
    };
}

inline M4
M4Transpose(const M4& s)
{
    auto& e = s.e;
    return {
        e[0][0], e[1][0], e[2][0], e[3][0],
        e[0][1], e[1][1], e[2][1], e[3][1],
        e[0][2], e[1][2], e[2][2], e[3][2],
        e[0][3], e[1][3], e[2][3], e[3][3]
    };
}

inline M3
M3Adj(const M3& s)
{
    return M3Transpose(M3Cofactors(s));
}

inline M4
M4Adj(const M4& s)
{
    return M4Transpose(M4Cofactors(s));
}

inline M3
operator*(const M3& l, const f32 r)
{
    M3 m {};

    for (int i = 0; i < 9; ++i)
        m.d[i] = l.d[i] * r;

    return m;
}

inline M4
operator*(const M4& l, const f32 r)
{
    M4 m {};

    for (int i = 0; i < 16; ++i)
        m.d[i] = l.d[i] * r;

    return m;
}

inline M3&
operator*=(M3& l, const f32 r)
{
    for (int i = 0; i < 9; ++i)
        l.d[i] *= r;

    return l;
}

inline M4&
operator*=(M4& l, const f32 r)
{
    for (int i = 0; i < 16; ++i)
        l.d[i] *= r;

    return l;
}

inline M3
operator*(const f32 l, const M3& r)
{
    return r * l;
}

inline M4
operator*(const f32 l, const M4& r)
{
    return r * l;
}

inline M3
M3Inv(const M3& s)
{
    return (1.0f/M3Det(s)) * M3Adj(s);
}

inline M4
M4Inv(const M4& s)
{
    return (1.0f/M4Det(s)) * M4Adj(s);
}

inline M3
operator*(const M3& l, const M3& r)
{
    M3 m {};

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                m.e[i][j] += l.e[i][k] * r.e[k][j];

    return m;
}

inline M3&
operator*=(M3& l, const M3& r)
{
    return l = l * r;
}

inline M4
operator*(const M4& l, const M4& r)
{
    M4 m {};

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                m.e[i][j] += l.e[i][k] * r.e[k][j];

    return m;
}

inline M4&
operator*=(M4& l, const M4& r)
{
    return l = l * r;
}

inline bool
operator==(const M3& l, const M3& r)
{
    for (int i = 0; i < 9; ++i)
        if (!rEq(l.d[i], r.d[i]))
            return false;

    return true;
}

inline bool
operator==(const M4& l, const M4& r)
{
    for (int i = 0; i < 16; ++i)
        if (!rEq(l.d[i], r.d[i]))
            return false;

    return true;
}

} /* namespace math */
} /* namespace adt */

#ifdef ADT_FMTLIB_INCLUDED
    #include <fmt/base.h>

template<>
class fmt::formatter<adt::math::V2>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::V2& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}]",
            s.e[0], s.e[1]
        );
    }
};

template<>
class fmt::formatter<adt::math::V3>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::V3& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}, {:.3}]",
            s.e[0], s.e[1], s.e[2]
        );
    }
};

template<>
class fmt::formatter<adt::math::V4>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::V4& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}, {:.3}, {:.3}]",
            s.e[0], s.e[1], s.e[2], s.e[3]
        );
    }
};

template<>
class fmt::formatter<adt::math::M2>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::M2& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}"
            "\n\t {:.3}, {:.3}]",
            s.e[0][0], s.e[0][1],
            s.e[1][0], s.e[1][1]
        );
    }
};

template<>
class fmt::formatter<adt::math::M3>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::M3& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}, {:.3}"
            "\n\t {:.3}, {:.3}, {:.3}"
            "\n\t {:.3}, {:.3}, {:.3}]",
            s.e[0][0], s.e[0][1], s.e[0][2],
            s.e[1][0], s.e[1][1], s.e[1][2],
            s.e[2][0], s.e[2][1], s.e[2][2]
        );
    }
};

template<>
class fmt::formatter<adt::math::M4>
{
  public:
    constexpr auto
    parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename CONTEXT_T>
    constexpr auto format(const adt::math::M4& s, CONTEXT_T& ctx) const
    {
        return format_to(
            ctx.out(),
            "\n\t[{:.3}, {:.3}, {:.3}, {:.3}"
            "\n\t {:.3}, {:.3}, {:.3}, {:.3}"
            "\n\t {:.3}, {:.3}, {:.3}, {:.3}"
            "\n\t {:.3}, {:.3}, {:.3}, {:.3}]",
            s.e[0][0], s.e[0][1], s.e[0][2], s.e[0][3],
            s.e[1][0], s.e[1][1], s.e[1][2], s.e[1][3],
            s.e[2][0], s.e[2][1], s.e[2][2], s.e[2][3],
            s.e[3][0], s.e[3][1], s.e[3][2], s.e[3][3]
        );
    }
};

#endif
