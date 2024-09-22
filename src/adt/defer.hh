#pragma once

/* https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/ */

namespace adt
{

template<typename F>
struct __Defer
{
    F onScopeExit;

    __Defer(F f) : onScopeExit(f) {}

    ~__Defer() { onScopeExit(); }
};

template <typename F>
__Defer<F>
__deferFunc(F f)
{
    return __Defer<F>(f);
}

} /* namespace adt */

#define ADT_DEFER_1(x, y) x##y
#define ADT_DEFER_2(x, y) ADT_DEFER_1(x, y)
#define ADT_DEFER_3(x) ADT_DEFER_2(x, __COUNTER__)

#define defer(code) auto ADT_DEFER_3(__defer__) = adt::__deferFunc([&] { code; })
