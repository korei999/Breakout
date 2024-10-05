#pragma once

namespace adt
{

template<typename A, typename B>
struct Pair
{
    union {
        A first;
        A a;
        A x;
    };
    union {
        A second;
        A b;
        A y;
    };

    Pair() = default;
    Pair(const A& _a, const B& _b) : a(_a), b(_b) {}
};

} /* namespace adt */
