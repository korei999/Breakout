#include "types.hh"

namespace adt
{

template<typename T>
struct Result
{
    bool bHasValue = false;
    T data;

    constexpr Result() = default;

    constexpr Result(const T& x)
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
