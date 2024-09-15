#include "types.hh"

namespace adt
{

template<typename T>
struct Option
{
    bool bHasValue = false;
    T data;

    constexpr Option() = default;

    constexpr Option(const T& x)
    {
        bHasValue = true;
        data = x;
    }

    constexpr explicit operator bool() const
    {
        return this->bHasValue;
    }
};

} /* namespace adt */
