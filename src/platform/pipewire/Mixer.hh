#pragma once

#include "audio.hh"
#include "adt/Array.hh"

#include <threads.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

namespace platform
{
namespace pipewire
{

constexpr u32 MAX_TRACK_COUNT = 1024;

struct Mixer;

void MixerInit(Mixer* s, int argc, char** argv);
void MixerDestroy(Mixer* s);
void MixerAdd(Mixer* s, audio::Track t);

inline const audio::MixerInterface __PwMixerVTable {
    .init = decltype(audio::MixerInterface::init)(MixerInit),
    .destroy = decltype(audio::MixerInterface::destroy)(MixerDestroy),
    .add = decltype(audio::MixerInterface::add)(MixerAdd),
};

struct Mixer
{
    audio::Mixer base;
    u32 sampleRate = 48000;
    u8 channels = 2;
    enum spa_audio_format eformat;

    static const pw_stream_events s_streamEvents;
    pw_core* pCore = nullptr;
    pw_context* pCtx = nullptr;
    pw_main_loop* pLoop = nullptr;
    pw_stream* pStream = nullptr;
    u32 lastNFrames = 0;

    mtx_t mtxAdd {};
    Array<audio::Track> aTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(Allocator* pA) : base {&__PwMixerVTable}, aTracks(pA, MAX_TRACK_COUNT) {}
};

} /* namespace pipewire */
} /* namespace platform */
