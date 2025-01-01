#pragma once

#include "adt/types.hh"

using namespace adt;

namespace audio
{

constexpr u64 CHUNK_SIZE = 0x4000; /* big enough */
constexpr u32 MAX_TRACK_COUNT = 8;

extern f32 g_globalVolume;

struct Track
{
    s16* pData = nullptr;
    u32 pcmPos = 0;
    u32 pcmSize = 0;
    u8 nChannels = 0;
    bool bRepeat = false;
    f32 volume = 0.0f;
};

/* Platrform abstracted audio interface */
struct IMixer
{
    bool m_bPaused = false;
    bool m_bMuted = false;
    bool m_bRunning = true;
    f32 m_volume = 0.5f;

    virtual void start() = 0;
    virtual void destroy() = 0;
    virtual void add(Track t) = 0;
    virtual void addBackground(Track t) = 0;
};

struct DummyMixer : IMixer
{
    virtual void start() override final {};
    virtual void destroy() override final {};
    virtual void add([[maybe_unused]] Track t) override final {};
    virtual void addBackground([[maybe_unused]] Track t) override final {};
};

} /* namespace audio */
