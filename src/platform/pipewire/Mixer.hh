#pragma once

#include "audio.hh"
#include "adt/Vec.hh"

#include <threads.h>

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#elif defined __GNUC__
    #pragma GCC diagnostic pop
#endif

namespace platform
{
namespace pipewire
{

struct Mixer;

void MixerInit(Mixer* s);
void MixerDestroy(Mixer* s);
void MixerAdd(Mixer* s, audio::Track t);
void MixerAddBackground(Mixer* s, audio::Track t);

inline const audio::MixerInterface __PwMixerVTable {
    .init = decltype(audio::MixerInterface::init)(MixerInit),
    .destroy = decltype(audio::MixerInterface::destroy)(MixerDestroy),
    .add = decltype(audio::MixerInterface::add)(MixerAdd),
    .addBackground = decltype(audio::MixerInterface::addBackground)(MixerAddBackground),
};

struct Mixer
{
    audio::Mixer super {};
    u32 sampleRate = 48000;
    u8 channels = 2;
    enum spa_audio_format eformat {};

    pw_core* pCore {};
    pw_context* pCtx {};
    pw_thread_loop* pThrdLoop {};
    pw_stream* pStream = nullptr;
    u32 lastNFrames {};

    mtx_t mtxAdd {};
    Vec<audio::Track> aTracks {};
    u32 currentBackgroundTrackIdx {};
    Vec<audio::Track> aBackgroundTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(Allocator* pA)
        : super(&__PwMixerVTable),
          aTracks(pA, audio::MAX_TRACK_COUNT),
          aBackgroundTracks(pA, audio::MAX_TRACK_COUNT) {}
};

} /* namespace pipewire */
} /* namespace platform */

namespace audio
{

inline void MixerInit(platform::pipewire::Mixer* s) { platform::pipewire::MixerInit(s); }
inline void MixerDestroy(platform::pipewire::Mixer* s) { platform::pipewire::MixerDestroy(s); }
inline void MixerAdd(platform::pipewire::Mixer* s, Track t) { platform::pipewire::MixerAdd(s, t); }
inline void MixerAddBackground(platform::pipewire::Mixer* s, Track t) { platform::pipewire::MixerAddBackground(s, t); }

} /* namespace audio */
