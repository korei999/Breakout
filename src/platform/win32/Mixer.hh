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

void MixerInit(Mixer* s, int argc = {}, char** argv = {});
void MixerDestroy(Mixer* s);
void MixerAdd(Mixer* s, audio::Track t);
void MixerAddBackground(Mixer* s, audio::Track t);

struct Mixer
{
    audio::IMixer super;
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    mtx_t mtxAdd {};
    Vec<audio::Track> aTracks {};
    u32 currentBackgroundTrackIdx = 0;
    Vec<audio::Track> aBackgroundTracks {};

    thrd_t threadLoop {};

    Mixer() = default;
    Mixer(IAllocator* pA);
};

} /* namespace win32 */
} /* namespace platform */

namespace audio
{


inline void MixerInit(platform::win32::Mixer* s, int argc, char** argv) { platform::win32::MixerInit(s, {}, {}); }
inline void MixerDestroy(platform::win32::Mixer* s) { platform::win32::MixerDestroy(s); }
inline void MixerAdd(platform::win32::Mixer* s, audio::Track t) { platform::win32::MixerAdd(s, t); }
inline void MixerAddBackground(platform::win32::Mixer* s, audio::Track t) { platform::win32::MixerAddBackground(s, t); }

} /* namespace audio */
