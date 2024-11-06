#include "test.hh"

#include "adt/ThreadPool.hh"
#include "adt/defer.hh"
#include "adt/guard.hh"
#include "adt/math.hh"
#include "adt/logs.hh"

using namespace adt;

namespace test
{

void
math()
{
    constexpr auto m3I = math::M3Iden();
    constexpr auto m4I = math::M4Iden();

    constexpr math::M3 a {
        1, 0, 1,
        0, 2, 1,
        1, 1, 1
    };

    auto aInv = math::M3Inv(a);
    assert((a * aInv) == m3I);

    constexpr math::M4 a4 {
        1, 0, 0, 1,
        0, 2, 1, 2,
        2, 1, 0, 1,
        2, 0, 1, 4
    };

    constexpr math::M3 a4m3 {
        1, 0, 0,
        0, 2, 1,
        2, 1, 0
    };

    auto a4Inv = math::M4Inv(a4);
    assert((a4 * a4Inv) == m4I);

    math::M3 m3FromM4 = math::M4ToM3(a4);
    assert(a4m3 == m3FromM4);

    math::M4 t2 {
        0, 0, 0, 1,
        8, 4, 2, 1,
        343, 49, 7, 1,
        1000, 100, 10, 1
    };
    math::V4 t2r {
        1, 1.5, 3, 1
    };
    math::V4 t2Exp {-0.012797616, 0.12232144, 0.05654758, 1};

    auto t2l = math::M4Inv(t2) * t2r;
    assert(t2Exp == t2l);

    LOG_GOOD("'math' passed\n");
}

mtx_t mtxLocks;

static int
lockGuard(int what)
{
    guard::Mtx lock(&mtxLocks);

    if (what < 0) 
    {
        return what * what;
    }
    else
    {
        return what + 5;
    }
}

void
locks()
{
    mtx_init(&mtxLocks, mtx_plain);
    defer(mtx_destroy(&mtxLocks));

    lockGuard(ADT_GET_NCORES());
    lockGuard(-2);

    LOG_GOOD("'locks' passed\n");
}

} /* namespace test */
