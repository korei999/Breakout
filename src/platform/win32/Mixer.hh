#pragma once

#include "audio.hh"
#include "adt/Vec.hh"

#include <xaudio2.h>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN 1
#endif
    #ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#undef near
#undef far

#include <threads.h>


namespace platform
{
namespace win32
{

struct Mixer;

void MixerInit(Mixer* s, int argc, char** argv);
void MixerDestroy(Mixer* s);
void MixerAdd(Mixer* s, audio::Track t);
void MixerAddBackground(Mixer* s, audio::Track t);

inline const audio::MixerInterface __XAudio2MixerVTable {
    .init = decltype(audio::MixerInterface::init)(MixerInit),
    .destroy = decltype(audio::MixerInterface::destroy)(MixerDestroy),
    .add = decltype(audio::MixerInterface::add)(MixerAdd),
    .addBackground = decltype(audio::MixerInterface::addBackground)(MixerAddBackground),
};

struct Mixer
{
    audio::Mixer base;
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    mtx_t mtxAdd {};
    Vec<audio::Track> aTracks {};
    u32 currentBackgroundTrackIdx = 0;
    Vec<audio::Track> aBackgroundTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(Allocator* pA) : base {&__XAudio2MixerVTable}, aTracks(pA, audio::MAX_TRACK_COUNT), aBackgroundTracks(pA, audio::MAX_TRACK_COUNT) {}
};

} /* namespace win32 */
} /* namespace platform */

namespace audio
{

inline void MixerInit(platform::win32::Mixer* s, int argc, char** argv) { platform::win32::MixerInit(s, argc, argv); }
inline void MixerDestroy(platform::win32::Mixer* s) { platform::win32::MixerDestroy(s); }
inline void MixerAdd(platform::win32::Mixer* s, audio::Track t) { platform::win32::MixerAdd(s, t); }
inline void MixerAddBackground(platform::win32::Mixer* s, audio::Track t) { platform::win32::MixerAddBackground(s, t); }

} /* namespace audio */
