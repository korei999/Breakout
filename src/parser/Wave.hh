#pragma once

#include "Bin.hh"
#include "audio.hh"

namespace parser
{

/* .wav audio file parser */
struct Wave;

void WaveParse(Wave* s);
inline bool WaveLoadFile(Wave* s, String path);
inline int WaveSubmit(void* pArg);

enum WAVE_FORMATS : s8
{
    F32 = -2, S16, S32
};

struct Wave
{
    Bin bin;
    s16* pPcmData = nullptr;
    u64 pcmSize = 0;
    u8 nChannels = 0;
    u32 sampleRate = 0;

    Wave() = default;
    Wave(IAllocator* pA) : bin(pA) {}
};

struct WaveLoadArg
{
    Wave* s;
    String path;
};

inline bool
WaveLoadFile(Wave* s, String path)
{
    return BinLoadFile(&s->bin, path);
}

inline audio::Track
WaveGetTrack(Wave* s, bool bRepeat, f32 vol)
{
    return audio::Track {
        .pData = s->pPcmData,
        .pcmPos = 0,
        .pcmSize = u32(s->pcmSize),
        .nChannels = s->nChannels,
        .bRepeat = bRepeat,
        .volume = vol
    };
}

inline int
WaveSubmit(void* pArg)
{
    auto a = *(WaveLoadArg*)pArg;
    WaveLoadFile(a.s, a.path);
    WaveParse(a.s);

    return 0;
}

} /* namespace parser */
