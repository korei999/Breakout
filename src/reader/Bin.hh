#pragma once

#include "adt/IAllocator.hh"
#include "adt/String.hh"
#include "adt/file.hh"

#include <bit>

using namespace adt;

namespace reader
{

template <typename T>
ADT_NO_UB inline T
readTypeBytes(String s, u32 at)
{
    return *(T*)(&s[at]);
}

struct Bin
{
    IAllocator* m_pAlloc;
    String m_sFile;
    String m_sPath;
    u32 m_pos;

    /* */

    Bin() = default;
    Bin(IAllocator* p) : m_pAlloc(p) {}

    /* */

    char& operator[](u32 i) { return m_sFile[i]; };

    bool load(String sPath);
    void skipBytes(u32 n);
    String readString(u32 bytes);
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

    m_sFile = rs.data;

    return m_sFile.data() != nullptr;
}

inline void 
Bin::skipBytes(u32 n)
{
    m_pos += n;
}

inline String
Bin::readString(u32 bytes)
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
    return std::byteswap(read16());
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
    return std::byteswap(read32());
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
    return std::byteswap(read64());
}

inline bool
Bin::finished()
{
    return m_pos >= m_sFile.getSize();
}

} /* namespace reader */
