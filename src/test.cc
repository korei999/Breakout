#include "test.hh"

#include "adt/math.hh"

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
}

} /* namespace test */
