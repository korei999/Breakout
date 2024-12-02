#pragma once

#include "adt/types.hh"

using namespace adt;

namespace audio
{

constexpr u64 CHUNK_SIZE = 0x4000; /* big enough */
constexpr u32 MAX_TRACK_COUNT = 8;

extern f32 g_globalVolume;

/* Platrform abstracted audio interface */
struct IMixer;
struct Track;

struct MixerVTable
{
    void (*start)(IMixer*);
    void (*destroy)(IMixer*);
    void (*add)(IMixer*, Track);
    void (*addBackground)(IMixer*, Track);
};

struct Track
{
    s16* pData = nullptr;
    u32 pcmPos = 0;
    u32 pcmSize = 0;
    u8 nChannels = 0;
    bool bRepeat = false;
    f32 volume = 0.0f;
};

struct IMixer
{
    const MixerVTable* pVTable;
    bool bPaused = false;
    bool bMuted = false;
    bool bRunning = true;
    f32 volume = 0.5f;
};

ADT_NO_UB constexpr void MixerStart(IMixer* s) { s->pVTable->start(s); }
ADT_NO_UB constexpr void MixerDestroy(IMixer* s) { s->pVTable->destroy(s); }
ADT_NO_UB constexpr void MixerAdd(IMixer* s, Track t) { s->pVTable->add(s, t); }
ADT_NO_UB constexpr void MixerAddBackground(IMixer* s, Track t) { s->pVTable->addBackground(s, t); }

struct DummyMixer
{
    IMixer super;

    constexpr DummyMixer();
};

constexpr void
DummyMixerStart([[maybe_unused]] DummyMixer* s)
{
    //
}

constexpr void
DummyMixerDestroy([[maybe_unused]] DummyMixer* s)
{
    //
}

constexpr void
DummyMixerAdd([[maybe_unused]] DummyMixer* s, [[maybe_unused]] Track t)
{
    //
}

constexpr void
DummyMixerAddBackground([[maybe_unused]] DummyMixer* s, [[maybe_unused]] Track t)
{
    //
}

inline const MixerVTable inl_DummyMixerVTable {
    .start = decltype(MixerVTable::start)(DummyMixerStart),
    .destroy = decltype(MixerVTable::destroy)(DummyMixerDestroy),
    .add = decltype(MixerVTable::add)(DummyMixerAdd),
    .addBackground = decltype(MixerVTable::addBackground)(DummyMixerAddBackground)
};

constexpr
DummyMixer::DummyMixer()
    : super {&inl_DummyMixerVTable} {}

} /* namespace audio */
