#pragma once

#include "adt/types.hh"

using namespace adt;

namespace audio
{

constexpr u64 CHUNK_SIZE = 0x4000; /* big enough */

/* Platrform abstracted audio interface */
struct Mixer;
struct Track;

struct MixerInterface
{
    void (*init)(Mixer* s, int argc, char** argv);
    void (*destroy)(Mixer*);
    void (*add)(Mixer*, Track);
};

struct Track
{
    f32* pData = nullptr;
    u32 pcmPos = 0;
    u32 pcmSize = 0;
    u8 nChannels = 0;
};

struct Mixer
{
    const MixerInterface* pVTable;
    bool bPaused = false;
    bool bMuted = false;
    bool bRunning = true;
    f32 volume = 0.5f;
};

ADT_NO_UB constexpr void MixerInit(Mixer* s, int argc, char** argv) { s->pVTable->init(s, argc, argv); }
ADT_NO_UB constexpr void MixerDestroy(Mixer* s) { s->pVTable->destroy(s); }
ADT_NO_UB constexpr void MixerAdd(Mixer* s, Track t) { s->pVTable->add(s, t); } /* call directly? */

} /* namespace audio */
