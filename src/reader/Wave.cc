#include "Wave.hh"

#include "adt/logs.hh"

namespace reader
{

/* https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html */

void
Wave::parse()
{
    [[maybe_unused]] String ckID = m_bin.readString(4);
    assert(ckID == "RIFF" && "not a Wave header");

    [[maybe_unused]] u32 ckSize = m_bin.read32();

    [[maybe_unused]] String waveid = m_bin.readString(4);
    assert(waveid == "WAVE" && "not a Wave header");

    /* FMT CHUNK */
    [[maybe_unused]] String fmtCkID = m_bin.readString(4);
    assert(fmtCkID == "fmt " && "not a Wave header");

    [[maybe_unused]] u32 fmtCkSize = m_bin.read32();
    assert(fmtCkSize == 16 || fmtCkSize == 18 || fmtCkSize == 40);

    [[maybe_unused]] s16 wFormatTag = m_bin.read16();
    if (wFormatTag != 1 && wFormatTag != 3)
    {
        LOG_WARN("wFormatTag: '{:#x}' unsupported Wave format\n", wFormatTag);
        return;
    }

    s16 nChannels = m_bin.read16();
    u32 nSamplesPerSec = m_bin.read32();
    [[maybe_unused]] s32 nAvgBytesPerSec = m_bin.read32();
    [[maybe_unused]] s16 nBlockAlign = m_bin.read16();

    [[maybe_unused]] u16 wBitsPerSample = m_bin.read16();

    String subchunk2ID = m_bin.readString(4);

    String data {};
    if (subchunk2ID != "data")
    {
        /* skip metadata */
        String __data {};
        while (__data != "data" && m_bin.m_pos + 4 < m_bin.m_sFile.getSize())
        {
            __data = m_bin.readString(4);
            m_bin.m_pos -= 3;

        }
        m_bin.m_pos -= 1;

        data = m_bin.readString(4);
    }
    else
    {
        /* TODO: qfact LIST  */
    }

    if (data != "data")
    {
        LOG_WARN("error loading wave file (no 'data' chunk)\n");
        return;
    }

    u32 subchunk2Size = m_bin.read32();

    m_sampleRate = nSamplesPerSec;
    m_nChannels = nChannels;
    m_pPcmData = reinterpret_cast<s16*>(&m_bin.m_sFile[m_bin.m_pos]);
    m_pcmSize = subchunk2Size / sizeof(s16);

#ifdef D_WAVE
    LOG_OK("'%.*s':\n", bin.sPath.size, bin.sPath.pData);
    LOG_OK("ckID: '%.*s'\n", ckID.size, ckID.pData);
    LOG_OK("ckSize: %u\n", ckSize);
    LOG_OK("waveid: '%.*s'\n", waveid.size, waveid.pData);
    LOG_OK("fmtCkID: '%.*s'\n", fmtCkID.size, fmtCkID.pData);
    LOG_OK("fmtCkSize: %u\n", fmtCkSize);
    LOG_OK("wFormatTag: %p\n", reinterpret_cast<void*>(wFormatTag));
    LOG_OK("nChannels: %d\n", nChannels);
    LOG_OK("nSamplesPerSec: %u\n", nSamplesPerSec);
    LOG_OK("nAvgBytesPerSec: %u\n", nAvgBytesPerSec);
    LOG_OK("nBlockAlign: %u\n", nBlockAlign);
    LOG_OK("wBitsPerSample: %u\n", wBitsPerSample);
    LOG_OK("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);
    LOG_OK("data: '%.*s'\n", data.size, data.pData);
    LOG_OK("subchunk2Size: %u\n", subchunk2Size);
    LOG_OK("fileSize: %u, pos: %u\n\n", bin.sFile.size, bin.pos);
#endif
}

} /* namespace reader */
