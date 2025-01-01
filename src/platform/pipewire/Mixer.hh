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

class Mixer : public audio::IMixer
{
    u32 m_sampleRate = 48000;
    u8 m_channels = 2;
    enum spa_audio_format m_eformat {};

    pw_thread_loop* m_pThrdLoop {};
    pw_stream* m_pStream = nullptr;
    u32 m_lastNFrames {};

    mtx_t m_mtxAdd {};
    Vec<audio::Track> m_aTracks {};
    u32 m_currentBackgroundTrackIdx {};
    Vec<audio::Track> m_aBackgroundTracks {};

    /* */

public:
    Mixer() = default;
    Mixer(IAllocator* pA);

    /* */ 

    virtual void start() override final;
    virtual void destroy() override final;
    virtual void add(audio::Track t) override final;
    virtual void addBackground(audio::Track t) override final;

    /* */

    static void* getOnProcessMethod() { return methodPointer(&Mixer::onProcess); }

    /* */
private:
    void writeFrames(void* pBuff, u32 nFrames);
    void onProcess();
};

} /* namespace pipewire */
} /* namespace platform */
