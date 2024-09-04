#pragma once

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <Windows.h>
#include <xaudio2.h>
#include <threads.h>

#include "audio.hh"
#include "adt/Array.hh"

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
    Array<audio::Track> aTracks {};
    u32 currentBackgroundTrackIdx = 0;
    Array<audio::Track> aBackgroundTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(Allocator* pA) : base {&__XAudio2MixerVTable}, aTracks(pA, audio::MAX_TRACK_COUNT), aBackgroundTracks(pA, audio::MAX_TRACK_COUNT) {}
};

} /* namespace win32 */
} /* namespace platform */
