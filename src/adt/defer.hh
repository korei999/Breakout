#pragma once

/* https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/ */

namespace adt
{

template<typename F>
struct Defer
{
    Defer(F f) : onScopeExit(f) {}
    ~Defer() { onScopeExit(); }

private:
    F onScopeExit;
};

template<typename F>
Defer<F>
deferFunc(F f)
{
    return Defer<F>(f);
}

} /* namespace adt */

#define ADT_DEFER_1(x, y) x##y
#define ADT_DEFER_2(x, y) ADT_DEFER_1(x, y)
#define ADT_DEFER_3(x) ADT_DEFER_2(x, __COUNTER__)

#define adtDefer(code) auto ADT_DEFER_3(__defer__) = adt::deferFunc([&] { code; })

#ifndef ADT_DEFER_ONLY
    #define defer(code) adtDefer(code)
#endif
