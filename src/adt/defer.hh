#pragma once

/* https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/ */

namespace adt
{

template<typename CLOSURE>
class Defer
{
    CLOSURE onScopeExit;

public:
    Defer(CLOSURE f) : onScopeExit(f) {}
    ~Defer() { onScopeExit(); }
};

} /* namespace adt */

/* create unique name with ## and __COUNTER__ */
#define ADT_DEFER_1(x, y) x##y##__
#define ADT_DEFER_2(x, y) ADT_DEFER_1(x, y)
#define ADT_DEFER_3(x) ADT_DEFER_2(x, __COUNTER__)

#define ADT_DEFER(code) auto ADT_DEFER_3(__clDefer) = adt::Defer([&] { code; })

#ifndef ADT_DEFER_ONLY
    #define defer(code) ADT_DEFER(code)
#endif
