#pragma once

#include "String.hh"

#include <assert.h>
#include <math.h>

namespace adt
{
namespace format
{

inline u32
digitCount(s64 x)
{
    return u32(log10(x)) + 1;
}

/* https://stackoverflow.com/a/54481936 */

inline u32
digitCount(f64 x)
{
    int digits = 0;
    f64 orig = x;
    long num2 = x;

    while (num2 > 0)
    {
        digits++;
        num2 = num2 / 10;
    }

    if (orig == 0) digits = 1;

    x = orig;
    f64 noFloat;
    noFloat = orig * (pow(10, (16 - digits)));
    s64 total = s64(noFloat);
    int noOfDigits, extraZeroes = 0;

    for (int i = 0; i < 16; i++)
    {
        int dig;
        dig = total % 10;
        total = total / 10;
        if (dig != 0)
            break;
        else
            extraZeroes++;
    }

    noOfDigits = 16 - extraZeroes;

    return noOfDigits;
}

inline String
toString(char* pBuf, [[maybe_unused]] u32 bufSize, s64 x)
{
    s64 absX = llabs(x);
    s64 digits = s64(digitCount(absX));
    bool bPositive = x > 0 ? true : false;
    s64 i;

    assert(digits < bufSize && "bufSize is not big enough");

    String nStr(pBuf, digits);

    if (!bPositive)
    {
        nStr[0] = '-';
        nStr.size++;
        i = 1;
    }
    else i = 0;

    for (; i < nStr.size; i++)
        nStr[i] = (absX / s64(pow(10, (nStr.size - i - 1))) % 10) + '0';

    return nStr;
}

inline String
toString(char* pBuf, u32 bufSize, s32 x)
{
    return toString(pBuf, bufSize, s64(x));
}

inline String
toString(char* pBuf, u32 bufSize, s16 x)
{
    return toString(pBuf, bufSize, s64(x));
}

inline String
toString(char* pBuf, u32 bufSize, s8 x)
{
    return toString(pBuf, bufSize, s64(x));
}

inline String
toString(String str, s64 x)
{
    return toString(str.pData, str.size, s64(x));
}

inline String
toString(String str, s32 x)
{
    return toString(str.pData, str.size, s64(x));
}

inline String
toString(String str, s16 x)
{
    return toString(str.pData, str.size, s64(x));
}

inline String
toString(String str, s8 x)
{
    return toString(str.pData, str.size, s64(x));
}

inline String
toString(char* pBuf, u32 bufSize, f64 x)
{
    s64 intPart = s64(x);
    f32 realPart = fabs(x) - llabs(intPart);
    s64 realPartInt = realPart * pow(10, digitCount(x));

    String sInt = toString(pBuf, bufSize, intPart);
    StringAppend(&sInt, ".");
    String sReal = toString(pBuf + sInt.size, bufSize - sInt.size + 1, realPartInt);
    StringAppend(&sInt, sReal);

    return sInt;
}

} /* namespace format */
} /* namespace adt */
