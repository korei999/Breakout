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
#include "adt/Thread.hh"

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
    IXAudio2* m_pXAudio2 = nullptr;
    IXAudio2SourceVoice* m_pSourceVoice = nullptr;

    Mutex m_mtxAdd {};
    Vec<audio::Track> m_aTracks {};
    u32 m_currentBackgroundTrackIdx = 0;
    Vec<audio::Track> m_aBackgroundTracks {};

    Thread m_threadLoop {};

    Mixer() = default;
    Mixer(IAllocator* pA);

    virtual void start() override final;
    virtual void destroy() override final;
    virtual void add(audio::Track t) override final;
    virtual void addBackground(audio::Track t) override final;
};

} /* namespace win32 */
} /* namespace platform */
