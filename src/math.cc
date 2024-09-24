#include "math.hh"

#include "adt/utils.hh"

#include <stdio.h>

namespace math
{

static M4 m4LookAtInternal(const V3& R, const V3& U, const V3& D, const V3& P);

V2::V2(const V3& v)
    : x(v.x), y(v.y) {}

V3::V3(const V2& v)
    : x(v.x), y(v.y) {}

V3::V3(const V4& v)
    : x(v.x), y(v.y), z(v.z) {}

V4::V4(const Qt& q)
    : x(q.x), y(q.y), z(q.z), w(q.s) {}

M4::M4(const M3& m)
    : p{m.p[0], m.p[1], m.p[2], 0,
        m.p[3], m.p[4], m.p[5], 0,
        m.p[6], m.p[7], m.p[8], 0,
        0,      0,      0,      1} {}

f32
V2Length(const V2& v)
{
    return hypotf(v.x, v.y);
}

f32
V3Length(const V3& v)
{
    return sqrtf(sq(v.x) + sq(v.y) + sq(v.z));
}

f32
V4Length(const V4& v)
{
    return sqrtf(sq(v.x) + sq(v.y) + sq(v.z) + sq(v.w));
}

V2
V2Norm(const V2& v)
{
    f32 len = V2Length(v);
    return V2 {v.x / len, v.y / len};
}

V3
V3Norm(const V3& v)
{
    f32 len = V3Length(v);
    return V3 {v.x / len, v.y / len, v.z / len};
}

V4
V4Norm(const V4& v)
{
    f32 len = V4Length(v);
    return {v.x / len, v.y / len, v.z / len, v.w / len};
}

V3
V3Norm(const V3& v, const f32 length)
{
    return V3 {v.x / length, v.y / length, v.z / length};
}

V3
V3Cross(const V3& l, const V3& r)
{
    return V3 {
        (l.y * r.z) - (r.y * l.z),
        (l.z * r.x) - (r.z * l.x),
        (l.x * r.y) - (r.x * l.y)
    };
}

V2
V2Clamp(const V2& x, const V2& min, const V2& max)
{
    V2 r {};

    f32 minX = utils::min(min.x, max.x);
    f32 minY = utils::min(min.y, max.y);

    f32 maxX = utils::max(min.x, max.x);
    f32 maxY = utils::max(min.y, max.y);

    r.x = utils::clamp(x.x, minX, maxX);
    r.y = utils::clamp(x.y, minY, maxY);

    return r;
}

V3
operator-(const V3& l, const V3& r)
{
    return V3 {
        l.x - r.x,
        l.y - r.y,
        l.z - r.z
    };
}

V2
operator-(const V2& l, const V2& r)
{
    return V2 {
        l.x - r.x,
        l.y - r.y
    };
}

V2
operator+(const V2& l, const V2& r)
{
    return V2 {
        l.x + r.x,
        l.y + r.y
    };
}

V2
operator+(const V2& l, f32 r)
{
    return {
        l.x + r,
        l.y + r
    };
}

V3
operator+(const V3& l, const V3& r)
{
    return V3 {
        l.x + r.x,
        l.y + r.y,
        l.z + r.z
    };
}

V2&
V2::operator+=(const V2& other)
{
    *this = *this + other;
    return *this;
}

V3&
V3::operator+=(const V3& other)
{
    *this = *this + other;
    return *this;
}

V3&
V3::operator-=(const V3& other)
{
    *this = *this - other;
    return *this;
}

V2
operator*(const V2& v, const f32 s)
{
    return V2 {
        v.x * s,
        v.y * s
    };
}

V2&
operator*=(V2& v, const f32 s)
{
    return v = v * s;
}

V3
operator*(const V3& v, const f32 s)
{
    return V3 {
        v.x * s,
        v.y * s,
        v.z * s
    };
}


V2
operator/(const V2& v, const f32 s)
{
    return {
        v.x / s,
        v.y / s
    };
}

V3
operator/(const V3& v, const f32 s)
{
    return {
        v.x / s,
        v.y / s,
        v.z / s
    };
}

V3&
operator/=(V3& v, const f32 s)
{
    return v = v / s;
}

V2&
operator/=(V2& v, const f32 s)
{
    return v = v / s;
}

bool
operator<(const V2& l, const V2& r)
{
    return V2Length(l) < V2Length(r) ? true : false;
}

bool
operator<=(const V2& l, const V2& r)
{
    return l < r || l == r;
}

bool
operator==(const V2& l, const V2& r)
{
    return l.x == r.x && l.y == r.y;
}

bool
operator>(const V2& l, const V2& r)
{
    return V2Length(l) > V2Length(r) ? true : false;
}

bool
operator>=(const V2& l, const V2& r)
{
    return l > r || l == r;
}

V3&
V3::operator*=(const f32 s)
{
    return *this = *this * s;
}

f32
V3Rad(const V3& l, const V3& r)
{
    return acosf(V3Dot(l, r) / (V3Length(l) * V3Length(r)));
}

f32
V2Dist(const V2& l, const V2& r)
{
    return sqrtf(sq(r.x - l.x) + sq(r.y - l.y));
}

f32
V3Dist(const V3& l, const V3& r)
{
    return sqrtf(sq(r.x - l.x) + sq(r.y - l.y) + sq(r.z - l.z));
}

f32
V2Dot(const V2& l, const V2& r)
{
    return (l.x * r.x) + (l.y * r.y);
}

f32
V3Dot(const V3& l, const V3& r)
{
    return (l.x * r.x) + (l.y * r.y) + (l.z * r.z);
}

f32
V4Dot(const V4& l, const V4& r)
{
    return (l.x * r.x) + (l.y * r.y) + (l.z * r.z) + (l.w * r.w);
}

V3
V3Color(const u32 hex)
{
    V3 t = MATH_COLOR3(hex);
    return t;
}

V4
V4Color(const u32 hex)
{
    V4 t = MATH_COLOR4(hex);
    return t;
}

M4&
M4::operator*=(const M4& other)
{
    *this = *this * other;
    return *this;
}

M4
operator*(const M4& l, const M4& r)
{
    M4 res {};

    for (int j = 0 ; j < 4; j++)
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                res.e[i][j] += l.e[k][j] * r.e[i][k];

    return res;
}

M4
M4Rot(const M4& m, const f32 th, const V3& ax)
{
    const f32 c = cosf(th);
    const f32 s = sinf(th);

    const f32 x = ax.x;
    const f32 y = ax.y;
    const f32 z = ax.z;

    M4 r {
        ((1 - c)*sq(x)) + c, ((1 - c)*x*y) - s*z, ((1 - c)*x*z) + s*y, 0,
        ((1 - c)*x*y) + s*z, ((1 - c)*sq(y)) + c, ((1 - c)*y*z) - s*x, 0,
        ((1 - c)*x*z) - s*y, ((1 - c)*y*z) + s*x, ((1 - c)*sq(z)) + c, 0,
        0,                   0,                   0,                   1
    };

    return m * r;
}

M4
M4RotX(const M4& m, const f32 angle)
{
    M4 axisX {
        1, 0,            0,           0,
        0, cosf(angle),  sinf(angle), 0,
        0, -sinf(angle), cosf(angle), 0,
        0, 0,            0,           1
    };

    return m * axisX;
}

M4
M4RotY(const M4& m, const f32 angle)
{
    M4 axisY {
        cosf(angle), 0,  -sinf(angle), 0,
        0,               1, 0,         0,
        sinf(angle), 0,  cosf(angle),  0,
        0,               0, 0,         1
    };

    return m * axisY;
}

M4
M4RotZ(const M4& m, const f32 angle)
{
    M4 axisZ {
        cosf(angle),  sinf(angle), 0, 0,
        -sinf(angle), cosf(angle), 0, 0,
        0,            0,           1, 0,
        0,            0,           0, 1
    };

    return m * axisZ;
}

M4
M4Scale(const M4& m, const f32 s)
{
    M4 sm {
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, 1
    };

    return m * sm;
}

M4
M4Scale(const M4& m, const V3& s)
{
    M4 sm {
        s.x, 0,   0,   0,
        0,   s.y, 0,   0,
        0,   0,   s.z, 0,
        0,   0,   0,   1
    };

    return m * sm;
}

M4
M4Translate(const M4& m, const V3& tv)
{
    M4 tm {
        1,    0,    0,    0,
        0,    1,    0,    0,
        0,    0,    1,    0,
        tv.x, tv.y, tv.z, 1
    };

    return m * tm;
}

M4
M4Pers(const f32 fov, const f32 asp, const f32 n, const f32 f)
{
    /* b(back), l(left) are not needed if viewing volume is symmetric */
    f32 t = n * tanf(fov / 2);
    f32 r = t * asp;

    return M4 {
        n / r, 0,     0,                  0,
        0,     n / t, 0,                  0,
        0,     0,    -(f + n) / (f - n), -1,
        0,     0,    -(2*f*n) / (f - n),  0
    };
}

M4
M4Ortho(const f32 l, const f32 r, const f32 b, const f32 t, const f32 n, const f32 f)
{
    return M4 {
        2/(r-l),       0,            0,           0,
        0,             2/(t-b),      0,           0,
        0,             0,           -2/(f-n),     0,
        -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1
    };
}

static M4
m4LookAtInternal(const V3& R, const V3& U, const V3& D, const V3& P)
{
    M4 m0 {
        R.x,  U.x,  D.x,  0,
        R.y,  U.y,  D.y,  0,
        R.z,  U.z,  D.z,  0,
        0,    0,    0,    1
    };

    return (M4Translate(m0, {-P.x, -P.y, -P.z}));
}

M4
M4LookAt(const V3& eyeV, const V3& centerV, const V3& upV)
{
    V3 camDir = V3Norm(eyeV - centerV);
    V3 camRight = V3Norm(V3Cross(upV, camDir));
    V3 camUp = V3Cross(camDir, camRight);

    return m4LookAtInternal(camRight, camUp, camDir, eyeV);
}

M4
M4Transpose(const M4& m)
{
    auto e = m.e;
    return {
        e[0][0], e[1][0], e[2][0], e[3][0],
        e[0][1], e[1][1], e[2][1], e[3][1],
        e[0][2], e[1][2], e[2][2], e[3][2],
        e[0][3], e[1][3], e[2][3], e[3][3]
    };
}

M3
M3Transpose(const M3& m)
{
    auto e = m.e;
    return {
        e[0][0], e[1][0], e[2][0],
        e[0][1], e[1][1], e[2][1],
        e[0][2], e[1][2], e[2][2]
    };
}

String
M4ToString(Allocator* pAlloc, const M4& m, String prefix)
{
    auto e = m.e;
    auto r = StringAlloc(pAlloc, 128);

    snprintf(r.pData, r.size, "%.*s:\n\t%.3f %.3f %.3f %.3f\n\t%.3f %.3f %.3f %.3f\n\t%.3f %.3f %.3f %.3f\n\t%.3f %.3f %.3f %.3f\n",
                              prefix.size, prefix.pData,
                              e[0][0], e[0][1], e[0][2], e[0][3],
                              e[1][0], e[1][1], e[1][2], e[1][3],
                              e[2][0], e[2][1], e[2][2], e[2][3],
                              e[3][0], e[3][1], e[3][2], e[3][3]);

    return r;
}

String
M3ToString(Allocator* pAlloc, const M3& m, String prefix)
{
    auto e = m.e;
    auto r = StringAlloc(pAlloc, 128);

    snprintf(r.pData, r.size, "%.*s:\n\t%.3f %.3f %.3f\n\t%.3f %.3f %.3f\n\t%.3f %.3f %.3f\n",
                              prefix.size, prefix.pData,
                              e[0][0], e[0][1], e[0][2],
                              e[1][0], e[1][1], e[1][2],
                              e[2][0], e[2][1], e[2][2]);

    return r;
}

String
V4ToString(Allocator* pAlloc, const V4& v, String prefix)
{
    auto r = StringAlloc(pAlloc, 128);
    snprintf(r.pData, r.size, "%.*s:\n\t%.3f %.3f %.3f %.3f\n", prefix.size, prefix.pData, v.x, v.y, v.z, v.w);

    return r;
}

M3
M3Inverse(const M3& m)
{
    auto e = m.e;
    f32 det = e[0][0] * (e[1][1] * e[2][2] - e[2][1] * e[1][2]) -
              e[0][1] * (e[1][0] * e[2][2] - e[1][2] * e[2][0]) +
              e[0][2] * (e[1][0] * e[2][1] - e[1][1] * e[2][0]);
    f32 invdet = 1.0f / det;

    return {
        (e[1][1] * e[2][2] - e[2][1] * e[1][2]) * invdet,
        (e[0][2] * e[2][1] - e[0][1] * e[2][2]) * invdet,
        (e[0][1] * e[1][2] - e[0][2] * e[1][1]) * invdet,

        (e[1][2] * e[2][0] - e[1][0] * e[2][2]) * invdet,
        (e[0][0] * e[2][2] - e[0][2] * e[2][0]) * invdet,
        (e[1][0] * e[0][2] - e[0][0] * e[1][2]) * invdet,

        (e[1][0] * e[2][1] - e[2][0] * e[1][1]) * invdet,
        (e[2][0] * e[0][1] - e[0][0] * e[2][1]) * invdet,
        (e[0][0] * e[1][1] - e[1][0] * e[0][1]) * invdet
    };
}

M3 
M3Normal(const M3& m)
{
    return M3Transpose(M3Inverse(m));
}

Qt
QtAxisAngle(const V3& axis, f32 th)
{
    f32 sinTh = f32(sin(th / 2));

    return {
        axis.x * sinTh,
        axis.y * sinTh,
        axis.z * sinTh,
        f32(cos(th / 2))
    };
}

M4
QtRot(const Qt& q)
{
    auto& x = q.x;
    auto& y = q.y;
    auto& z = q.z;
    auto& s = q.s;

    return {
        1 - 2*y*y - 2*z*z, 2*x*y - 2*s*z,     2*x*z + 2*s*y,     0,
        2*x*y + 2*s*z,     1 - 2*x*x - 2*z*z, 2*y*z - 2*s*x,     0,
        2*x*z - 2*s*y,     2*y*z + 2*s*x,     1 - 2*x*x - 2*y*y, 0,
        0,                 0,                 0,                 1
    };
}

Qt
QtConj(const Qt& q)
{
    return {-q.x, -q.y, -q.z, q.s};
}

Qt
operator*(const Qt& l, const Qt& r)
{
    return {
        l.s*r.x + l.x*r.s + l.y*r.z - l.z*r.y,
        l.s*r.y - l.x*r.z + l.y*r.s + l.z*r.x,
        l.s*r.z + l.x*r.y - l.y*r.x + l.z*r.s,
        l.s*r.s - l.x*r.x - l.y*r.y - l.z*r.z,
    };
}

Qt
operator*=(Qt& l, const Qt& r)
{
    return l = l * r;
}

} /* namespace math */
