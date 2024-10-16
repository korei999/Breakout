#pragma once

#include "Allocator.hh"
#include "hash.hh"

#include <cstdio>

namespace adt
{

constexpr u32
nullTermStringSize(const char* str)
{
    u32 i = 0;
    if (!str) return i;

    while (str[i] != '\0')
        i++;

    return i;
}

/* just pointer + size, no allocations, use `StringAlloc()` for that */
struct String
{
    char* pData = nullptr;
    u32 size = 0;

    constexpr String() = default;
    constexpr String(char* sNullTerminated) : pData (sNullTerminated), size (nullTermStringSize(sNullTerminated)) {}
    constexpr String(const char* sNullTerminated) : pData (const_cast<char*>(sNullTerminated)), size (nullTermStringSize(sNullTerminated)) {}
    constexpr String(char* pStr, u32 len) : pData (pStr), size (len) {}

    constexpr char& operator[](u32 i) { return pData[i]; }
    constexpr const char& operator[](u32 i) const { return pData[i]; }

    struct It
    {
        char* p;

        constexpr It(char* pFirst) : p{pFirst} {}

        constexpr char& operator*() { return *p; }
        constexpr char* operator->() { return p; }

        constexpr It operator++() { p++; return *this; }
        constexpr It operator++(int) { char* tmp = p++; return tmp; }
        constexpr It operator--() { p--; return *this; }
        constexpr It operator--(int) { char* tmp = p--; return tmp; }

        friend constexpr bool operator==(It l, It r) { return l.p == r.p; }
        friend constexpr bool operator!=(It l, It r) { return l.p != r.p; }
    };

    constexpr It begin() { return {&this->pData[0]}; }
    constexpr It end() { return {&this->pData[this->size]}; }
    constexpr It rbegin() { return {&this->pData[this->size - 1]}; }
    constexpr It rend() { return {this->pData - 1}; }

    constexpr const It begin() const { return {&this->pData[0]}; }
    constexpr const It end() const { return {&this->pData[this->size]}; }
    constexpr const It rbegin() const { return {&this->pData[this->size - 1]}; }
    constexpr const It rend() const { return {this->pData - 1}; }
};

constexpr bool StringEndsWith(String l, String r);
constexpr bool operator==(const String& l, const String& r);
constexpr bool operator==(const String& l, const char* r);
constexpr bool operator!=(const String& l, const String& r);
constexpr s64 operator-(const String& l, const String& r);
constexpr u32 StringLastOf(String sv, char c);
constexpr String StringAlloc(Allocator* p, const char* str, u32 size);
constexpr String StringAlloc(Allocator* p, u32 size);
constexpr String StringAlloc(Allocator* p, const char* str);
constexpr String StringAlloc(Allocator* p, String s);
constexpr u64 hashFNV(String str);
constexpr String StringCat(Allocator* p, String l, String r);

template<> constexpr u64 hash::func<String>(String& str);
template<> constexpr u64 hash::func<const String>(const String& str);

constexpr bool
StringEndsWith(String l, String r)
{
    if (l.size < r.size)
        return false;

    for (int i = r.size - 1, j = l.size - 1; i >= 0; i--, j--)
        if (r[i] != l[j])
            return false;

    return true;
}

constexpr bool
operator==(const String& l, const String& r)
{
    if (l.size != r.size) return false;

    for (u32 i = 0; i < l.size; i++)
        if (l[i] != r[i]) return false;

    return true;
}

constexpr bool
operator==(const String& l, const char* r)
{
    auto sr = String(r);
    return l == sr;
}

constexpr bool
operator!=(const String& l, const String& r)
{
    return !(l == r);
}

constexpr s64
operator-(const String& l, const String& r)
{
    if (l.size < r.size) return -1;
    else if (l.size > r.size) return 1;

    s64 sum = 0;
    for (u32 i = 0; i < l.size; i++)
        sum += (l[i] - r[i]);

    return sum;
}

constexpr u32
StringLastOf(String sv, char c)
{
    for (int i = sv.size - 1; i >= 0; i--)
        if (sv[i] == c)
            return i;

    return NPOS;
}

constexpr String
StringAlloc(Allocator* p, const char* str, u32 size)
{
    char* pData = (char*)alloc(p, size + 1, sizeof(char));
    for (u32 i = 0; i < size; i++)
        pData[i] = str[i];
    pData[size] = '\0';

    return {pData, size};
}

constexpr String
StringAlloc(Allocator* p, u32 size)
{
    char* pData = (char*)alloc(p, size + 1, sizeof(char));
    return {pData, size};
}

constexpr String
StringAlloc(Allocator* p, const char* str)
{
    return StringAlloc(p, str, nullTermStringSize(str));
}

constexpr String
StringAlloc(Allocator* p, const String s)
{
    return StringAlloc(p, s.pData, s.size);
}

constexpr u64
hashFNV(const String str)
{
    return hash::fnv(str.pData, str.size);
}

constexpr String
StringCat(Allocator* p, const String l, const String r)
{
    u32 len = l.size + r.size;
    char* ret = (char*)alloc(p, len + 1, sizeof(char));

    u32 pos = 0;
    for (u32 i = 0; i < l.size; i++, pos++)
        ret[pos] = l[i];
    for (u32 i = 0; i < r.size; i++, pos++)
        ret[pos] = r[i];

    ret[len] = '\0';

    return {ret, len};
}

constexpr void
StringAppend(String* l, const String r)
{
    for (long i = l->size, j = 0; i < long(l->size + r.size); i++, j++)
        (*l)[i] = r[j];

    l->size += r.size;
}

constexpr void
StringTrimEnd(String* s)
{
    auto isWhiteSpace = [&](int i) -> bool {
        char c = s->pData[i];
        /*fprintf(stderr, "char: '%d'\n", c);*/
        if (c == '\n' || c == ' ' || c == '\r' || c == '\t' || c == '\0')
            return true;

        return false;
    };

    for (int i = s->size - 1; i >= 0; --i)
        if (isWhiteSpace(i))
        {
            s->pData[i] = 0;
            --s->size;
        }
        else break;
}

/* removes nextline character if it ends with one */
constexpr void
StringRemoveNLEnd(String* s)
{
    if (s->pData[s->size - 1] == '\n')
        s->pData[--s->size] = '\0';
    else if (s->pData[s->size - 1] == '\r' && s->pData[s->size - 2] == '\n')
        s->pData[s->size -= 2] = '\0';
}

template<>
constexpr u64
hash::func<String>(String& str)
{
    return fnv(str.pData, str.size);
}

template<>
constexpr u64
hash::func<const String>(const String& str)
{
    return fnv(str.pData, str.size);
}

} /* namespace adt */
