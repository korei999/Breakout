#pragma once

#include <math.h>

#include "adt/types.hh"

#ifdef LOGS
    #include "adt/String.hh"
#endif

using namespace adt;

namespace math
{

constexpr f64 PI = 3.14159265358979323846;
constexpr f32 PI_F = f32(PI);
constexpr f64 EPS = 0.000001;
constexpr f32 EPS_F = 0.00001;

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

#define MATH_COLOR4(hex)                                                                                               \
    {                                                                                                                  \
        ((hex >> 24) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

#define MATH_COLOR3(hex)                                                                                               \
    {                                                                                                                  \
        ((hex >> 16) & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 8)  & 0xFF) / 255.0f,                                                                                 \
        ((hex >> 0)  & 0xFF) / 255.0f                                                                                  \
    }

union V2;
union V3;
union V4;
union Qt;
union M3;

union V2
{
    struct {
        f32 x, y;
    };
    f32 e[2];

    V2() = default;
    V2(const V3& v);
    constexpr V2(f32 _x, f32 _y) : x(_x), y(_y) {}

    constexpr V2 operator-() { return {-x, -y}; }

    V2& operator+=(const V2& other);
};

union V3
{
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
    f32 e[3];

    V3() = default;
    explicit V3(const V2& v);
    V3(const V4& v);
    constexpr V3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}
    constexpr V3(u32 hex)
        : x(((hex >> 16) & 0xFF) / 255.0f),
          y(((hex >> 8)  & 0xFF) / 255.0f),
          z(((hex >> 0)  & 0xFF) / 255.0f) {}

    V3& operator+=(const V3& other);
    V3& operator-=(const V3& other);
    V3& operator*=(const f32 s);
};

union V4
{
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 e[4];

    V4() = default;
    constexpr V4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}
    V4(const Qt& q);
    V4(const V3& _v3, f32 _a) : x(_v3.x), y(_v3.y), z(_v3.z), w(_a) {}
};

union M4
{
    V4 v[4];
    f32 e[4][4];
    f32 p[16];

    M4() = default;
    constexpr M4(f32 _0, f32 _1, f32 _2, f32 _3,
                 f32 _4, f32 _5, f32 _6, f32 _7,
                 f32 _8, f32 _9, f32 _10, f32 _11,
                 f32 _12, f32 _13, f32 _14, f32 _15)
        : p{_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15} {}
    M4(const M3& m);

    M4& operator*=(const M4& other);
};

union M3
{
    V3 v[4];
    f32 e[3][3];
    f32 p[9];

    M3() = default;
    M3(const M4& m) : v{m.v[0], m.v[1], m.v[2]} {}
    constexpr M3(f32 _0, f32 _1, f32 _2, f32 _3, f32 _4, f32 _5, f32 _6, f32 _7, f32 _8)
        : p{_0, _1, _2, _3, _4, _5, _6, _7, _8} {}
};

union Qt
{
    struct {
        f32 x, y, z, s;
    };
    f32 p[4];

    Qt() = default;
    constexpr Qt(f32 _x, f32 _y, f32 _z, f32 _s) : x(_x), y(_y), z(_z), s(_s) {}
    constexpr Qt(V4 _v) : x(_v.x), y(_v.y), z(_v.z), s(_v.w) {}
    constexpr Qt(V3 _v, f32 _s) : x(_v.x), y(_v.y), z(_v.z), s(_s) {}
};

#ifdef LOGS
String M4ToString(Allocator* pAlloc, const M4& m, String prefix);
String M3ToString(Allocator* pAlloc, const M3& m, String prefix);
String M4ToString(Allocator* pAlloc, const M4& m, String prefix);
#endif

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

constexpr M3
M3Iden()
{
    return {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };
}

constexpr Qt
QtIden()
{
    return {0, 0, 0, 1};
}

f32 V2Length(const V2& v);
f32 V3Length(const V3& v);
f32 V3Length(const V2& v);
f32 V4Length(const V4& v);
V2 V2Norm(const V2& v);
V3 V3Norm(const V3& v); /* normilized (not length) */
V4 V4Norm(const V4& v); /* normilized (not length) */
V3 V3Norm(const V3& v, const f32 length); /* normilized (not length) */
V3 V3Cross(const V3& l, const V3& r);
V2 V2Clamp(const V2& x, const V2& min, const V2& max);
V2 operator-(const V2& l, const V2& r);
V3 operator-(const V3& l, const V3& r);
V2 operator+(const V2& l, const V2& r);
V2 operator+(const V2& l, f32 r);
V3 operator+(const V3& l, const V3& r);
V2 operator*(const V2& v, const f32 s);
V2& operator*=(V2& v, const f32 s);
V3 operator*(const V3& v, const f32 s);
V3 operator/(const V3& v, const f32 s);
V2 operator/(const V2& v, const f32 s);
V3& operator/=(V3& v, const f32 s);
V2& operator/=(V2& v, const f32 s);
bool operator<(const V2& l, const V2& r);
bool operator<=(const V2& l, const V2& r);
bool operator==(const V2& l, const V2& r);
bool operator>(const V2& l, const V2& r);
bool operator>=(const V2& l, const V2& r);
f32 V3Rad(const V3& l, const V3& r); /* degree between two vectors */
f32 V2Dist(const V2& l, const V2& r);
f32 V3Dist(const V3& l, const V3& r); /* distance between two points in space */
f32 V2Dot(const V2& l, const V2& r);
f32 V3Dot(const V3& l, const V3& r);
f32 V4Dot(const V4& l, const V4& r);
V3 V3Color(const u32 hex);
V4 V4Color(const u32 hex);

M4 operator*(const M4& l, const M4& r);
M4 M4Rot(const M4& m, const f32 th, const V3& ax);
M4 M4RotX(const M4& m, const f32 angle);
M4 M4RotY(const M4& m, const f32 angle);
M4 M4RotZ(const M4& m, const f32 angle);
M4 M4Scale(const M4& m, const f32 s);
M4 M4Scale(const M4& m, const V3& s);
M4 M4Translate(const M4& m, const V3& tv);
M4 M4Pers(const f32 fov, const f32 asp, const f32 n, const f32 f);
M4 M4Ortho(const f32 l, const f32 r, const f32 b, const f32 t, const f32 n, const f32 f);
M4 M4LookAt(const V3& eyeV, const V3& centerV, const V3& upV);
M4 M4Transpose(const M4& m);
M3 M3Transpose(const M3& m);
M3 M3Inverse(const M3& m);
M3 M3Normal(const M3& m);

Qt QtAxisAngle(const V3& axis, f32 th);
M4 QtRot(const Qt& q);
Qt QtConj(const Qt& q);
Qt operator*(const Qt& l, const Qt& r);
Qt operator*=(Qt& l, const Qt& r);


/* TODO: rewrite the whole library as header only */
inline V2
normalize(const V2& v)
{
    return V2Norm(v);
}

inline V3
normalize(const V3& v)
{
    return V3Norm(v);
}

inline V4
normalize(const V4& v)
{
    return V4Norm(v);
}

} /* namespace math */
