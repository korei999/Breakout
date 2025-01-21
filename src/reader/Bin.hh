#pragma once

#include "adt/IAllocator.hh"
#include "adt/String.hh"
#include "adt/file.hh"

using namespace adt;

namespace reader
{

template <typename T>
ADT_NO_UB inline T
readTypeBytes(String s, ssize at)
{
    return *(T*)(&s[at]);
}

inline constexpr u16
swapBytes(u16 x)
{
    return ((x & 0xff00u) >> 1 * 8) |
           ((x & 0x00ffu) << 1 * 8);
}

inline constexpr u32
swapBytes(u32 x)
{
    return ((x & 0xff000000u) >> 3 * 8) |
           ((x & 0x00ff0000u) >> 1 * 8) |
           ((x & 0x0000ff00u) << 1 * 8) |
           ((x & 0x000000ffu) << 3 * 8);
}

inline constexpr u64
swapBytes(u64 x)
{
    return ((x & 0xff00000000000000llu) >> 7 * 8) |
           ((x & 0x00ff000000000000llu) >> 5 * 8) |
           ((x & 0x0000ff0000000000llu) >> 2 * 8) |
           ((x & 0x000000ff00000000llu) >> 1 * 8) |
           ((x & 0x00000000ff000000llu) << 1 * 8) |
           ((x & 0x0000000000ff0000llu) << 3 * 8) |
           ((x & 0x000000000000ff00llu) << 5 * 8) |
           ((x & 0x00000000000000ffllu) << 7 * 8);
}

struct Bin
{
    IAllocator* m_pAlloc;
    String m_sFile;
    String m_sPath;
    ssize m_pos;

    /* */

    Bin() = default;
    Bin(IAllocator* p) : m_pAlloc(p) {}

    /* */

    char& operator[](ssize i) { return m_sFile[i]; };

    bool load(String sPath);
    void skipBytes(ssize n);
    String readString(ssize bytes);
    u8 read8();
    u16 read16();
    u16 read16Rev();
    u32 read32();
    u32 read32Rev();
    u64 read64();
    u64 read64Rev();
    bool finished();
};

inline bool 
Bin::load(String path)
{
    m_sPath = StringAlloc(m_pAlloc, path);
    m_pos = 0;

    Opt<String> rs = file::load(m_pAlloc, path);
    if (!rs) LOG_FATAL("error opening file: '{}'\n", path);

    m_sFile = rs.value();

    return m_sFile.data() != nullptr;
}

inline void 
Bin::skipBytes(ssize n)
{
    m_pos += n;
}

inline String
Bin::readString(ssize bytes)
{
    String ret(&m_sFile[m_pos], bytes);
    m_pos += bytes;
    return ret;
}

inline u8
Bin::read8()
{
    auto ret = readTypeBytes<u8>(m_sFile, m_pos);
    m_pos += 1;
    return ret;
}

inline u16
Bin::read16()
{
    auto ret = readTypeBytes<u16>(m_sFile, m_pos);
    m_pos += 2;
    return ret;
}

inline u16
Bin::read16Rev()
{
    /*return std::byteswap(read16());*/
    return swapBytes(read16());
}

inline u32
Bin::read32()
{
    auto ret = readTypeBytes<u32>(m_sFile, m_pos);
    m_pos += 4;
    return ret;
}

inline u32
Bin::read32Rev()
{
    /*return std::byteswap(read32());*/
    return swapBytes(read32());
}

inline u64
Bin::read64()
{
    auto ret = readTypeBytes<u64>(m_sFile, m_pos);
    m_pos += 8;
    return ret;
}

inline u64
Bin::read64Rev()
{
    /*return std::byteswap(read64());*/
    return swapBytes(read64());
}

inline bool
Bin::finished()
{
    return m_pos >= m_sFile.getSize();
}

} /* namespace reader */
