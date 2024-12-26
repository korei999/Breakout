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

struct Mixer : audio::IMixer
{
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

    /* */

    Mixer() = default;
    Mixer(IAllocator* pA);

    /* */ 

    virtual void start();
    virtual void destroy();
    virtual void add(audio::Track t);
    virtual void addBackground(audio::Track t);
};

} /* namespace pipewire */
} /* namespace platform */
