#pragma once

#include "Bin.hh"
#include "audio.hh"

namespace reader
{

/* .wav audio file parser */
struct Wave;

inline int WaveSubmit(void* pArg);

enum WAVE_FORMATS : s8
{
    F32 = -2, S16, S32
};

struct Wave
{
    Bin m_bin;
    s16* m_pPcmData = nullptr;
    u64 m_pcmSize = 0;
    u8 m_nChannels = 0;
    u32 m_sampleRate = 0;

    /* */

    Wave() = default;
    Wave(IAllocator* pA) : m_bin(pA) {}

    /* */

    void parse();
    bool load(String path) { return m_bin.load(path); }

    audio::Track
    getTrack(bool bRepeat, f32 vol)
    {
        return audio::Track {
            .pData = m_pPcmData,
            .pcmPos = 0,
            .pcmSize = u32(m_pcmSize),
            .nChannels = m_nChannels,
            .bRepeat = bRepeat,
            .volume = vol
        };
    }
};

struct WaveLoadArg
{
    Wave* s;
    String path;
};

inline int
WaveSubmit(void* pArg)
{
    auto a = *(WaveLoadArg*)pArg;
    a.s->load(a.path);
    a.s->parse();

    return 0;
}

} /* namespace reader */
