#pragma once

#include <xaudio2.h>
#undef near
#undef far
#undef NEAR
#undef FAR
#undef min
#undef max
#undef MIN
#undef MAX

#include "audio.hh"
#include "adt/Vec.hh"

#include <threads.h>

namespace platform
{
namespace win32
{

struct Mixer;

void MixerStart(Mixer* s);
void MixerDestroy(Mixer* s);
void MixerAdd(Mixer* s, audio::Track t);
void MixerAddBackground(Mixer* s, audio::Track t);

struct Mixer : public audio::IMixer
{
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    mtx_t mtxAdd {};
    Vec<audio::Track> aTracks {};
    u32 currentBackgroundTrackIdx = 0;
    Vec<audio::Track> aBackgroundTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(IAllocator* pA);

    virtual void start();
    virtual void destroy();
    virtual void add(audio::Track t);
    virtual void addBackground(audio::Track t);
};

} /* namespace win32 */
} /* namespace platform */
