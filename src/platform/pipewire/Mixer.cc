#include "Mixer.hh"

#include "adt/logs.hh"
#include "adt/utils.hh"
#include "app.hh"

#include <cmath>

#include <emmintrin.h>
#include <immintrin.h>

namespace platform
{
namespace pipewire
{

struct PwLockGuard
{
    pw_thread_loop* p;

    PwLockGuard() = delete;
    PwLockGuard(pw_thread_loop* _p) : p(_p) { pw_thread_loop_lock(_p); }
    ~PwLockGuard() { pw_thread_loop_unlock(p); }
};

static const pw_stream_events s_streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy {},
    .state_changed {},
    .control_info {},
    .io_changed {},
    .param_changed {},
    .add_buffer {},
    .remove_buffer {},
    .process = decltype(pw_stream_events::process)(Mixer::getOnProcessMethod()),
    .drained {},
    .command {},
    .trigger_done {},
};

Mixer::Mixer(IAllocator* pA)
    : m_aTracks(pA, audio::MAX_TRACK_COUNT),
      m_aBackgroundTracks(pA, audio::MAX_TRACK_COUNT)
{
    this->m_bRunning = true;
    this->m_bMuted = false;
    this->m_volume = 0.1f;

    this->m_sampleRate = 48000;
    this->m_channels = 2;
    this->m_eformat = SPA_AUDIO_FORMAT_S16_LE;

    mtx_init(&this->m_mtxAdd, mtx_plain);
}

void
Mixer::start()
{
    pw_init(&app::g_argc, &app::g_argv);

    u8 aBuff[1024] {};
    const spa_pod* aParams[1] {};
    spa_pod_builder b {};
    spa_pod_builder_init(&b, aBuff, sizeof(aBuff));

    this->m_pThrdLoop = pw_thread_loop_new("BreakoutThreadLoop", {});

    this->m_pStream = pw_stream_new_simple(
        pw_thread_loop_get_loop(this->m_pThrdLoop),
        "BreakoutAudioSource",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Game",
            nullptr
        ),
        &s_streamEvents,
        this
    );

    spa_audio_info_raw rawInfo {
        .format = this->m_eformat,
        .flags {},
        .rate = this->m_sampleRate,
        .channels = this->m_channels,
        .position {}
    };

    aParams[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &rawInfo);

    pw_stream_connect(
        this->m_pStream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_INACTIVE | PW_STREAM_FLAG_MAP_BUFFERS),
        aParams,
        utils::size(aParams)
    );

    pw_thread_loop_start(this->m_pThrdLoop);

    PwLockGuard tLock(this->m_pThrdLoop);
    pw_stream_set_active(this->m_pStream, true);
}

void
Mixer::destroy()
{
    {
        PwLockGuard tLock(this->m_pThrdLoop);
        pw_stream_set_active(this->m_pStream, true);
    }

    pw_thread_loop_stop(this->m_pThrdLoop);
    LOG_NOTIFY("pw_thread_loop_stop()\n");

    this->m_bRunning = false;

    pw_stream_destroy(this->m_pStream);
    pw_thread_loop_destroy(this->m_pThrdLoop);
    pw_deinit();

    this->m_aTracks.destroy();
    this->m_aBackgroundTracks.destroy();
    mtx_destroy(&this->m_mtxAdd);
}

void
Mixer::add(audio::Track t)
{
    mtx_lock(&this->m_mtxAdd);

    if (this->m_aTracks.getSize() < audio::MAX_TRACK_COUNT) this->m_aTracks.push(t);
    else LOG_WARN("MAX_TRACK_COUNT({}) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&this->m_mtxAdd);
}

void
Mixer::addBackground(audio::Track t)
{
    mtx_lock(&this->m_mtxAdd);

    if (this->m_aTracks.getSize() < audio::MAX_TRACK_COUNT) this->m_aBackgroundTracks.push(t);
    else LOG_WARN("MAX_TRACK_COUNT({}) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&this->m_mtxAdd);
}

void
Mixer::writeFrames(void* pBuff, u32 nFrames)
{
    __m128i_u* pSimdDest = (__m128i_u*)pBuff;

    for (u32 i = 0; i < nFrames / 4; ++i)
    {
        __m128i packed8Samples {};

        if (m_aBackgroundTracks.getSize() > 0)
        {
            auto& t = m_aBackgroundTracks[m_currentBackgroundTrackIdx];
            f32 vol = std::pow(t.volume * audio::g_globalVolume, 3.0f);

            if (t.pcmPos + 8 <= t.pcmSize)
            {
                auto what = _mm_set_epi16(
                    t.pData[t.pcmPos + 7] * vol,
                    t.pData[t.pcmPos + 6] * vol,
                    t.pData[t.pcmPos + 5] * vol,
                    t.pData[t.pcmPos + 4] * vol,
                    t.pData[t.pcmPos + 3] * vol,
                    t.pData[t.pcmPos + 2] * vol,
                    t.pData[t.pcmPos + 1] * vol,
                    t.pData[t.pcmPos + 0] * vol
                );

                packed8Samples = _mm_add_epi16(packed8Samples, what);

                t.pcmPos += 8;
            }
            else
            {
                t.pcmPos = 0;
                auto current = m_currentBackgroundTrackIdx + 1;
                if (current > m_aBackgroundTracks.getSize() - 1) current = 0;
                m_currentBackgroundTrackIdx = current;
            }
        }

        for (u32 i = 0; i < m_aTracks.getSize(); ++i)
        {
            auto& t = m_aTracks[i];
            f32 vol = powf(t.volume, 3.0f);

            if (t.pcmPos + 8 <= t.pcmSize)
            {
                auto what = _mm_set_epi16(
                    t.pData[t.pcmPos + 7] * vol,
                    t.pData[t.pcmPos + 6] * vol,
                    t.pData[t.pcmPos + 5] * vol,
                    t.pData[t.pcmPos + 4] * vol,
                    t.pData[t.pcmPos + 3] * vol,
                    t.pData[t.pcmPos + 2] * vol,
                    t.pData[t.pcmPos + 1] * vol,
                    t.pData[t.pcmPos + 0] * vol
                );

                packed8Samples = _mm_add_epi16(packed8Samples, what);

                t.pcmPos += 8;
            }
            else
            {
                if (t.bRepeat)
                {
                    t.pcmPos = 0;
                }
                else
                {
                    mtx_lock(&m_mtxAdd);
                    m_aTracks.popAsLast(i);
                    --i;
                    mtx_unlock(&m_mtxAdd);
                }
            }
        }

        _mm_storeu_si128(pSimdDest, packed8Samples);

        pSimdDest++;
    }
}

void
Mixer::onProcess()
{
    pw_buffer* pPwBuffer = pw_stream_dequeue_buffer(m_pStream);
    if (!pPwBuffer)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    auto pBuffData = pPwBuffer->buffer->datas[0];
    s16* pDest = (s16*)pBuffData.data;

    if (!pDest)
    {
        LOG_WARN("dst == nullptr\n");
        return;
    }

    u32 stride = sizeof(s16) * m_channels;
    u32 nFrames = pBuffData.maxsize / stride;
    if (pPwBuffer->requested) nFrames = SPA_MIN(pPwBuffer->requested, (u64)nFrames);

    if (nFrames > 1024*4) nFrames = 1024*4; /* limit to arbitrary number */

    m_lastNFrames = nFrames;

    writeFrames(pDest, nFrames);

    pBuffData.chunk->offset = 0;
    pBuffData.chunk->stride = stride;
    pBuffData.chunk->size = nFrames * stride;

    pw_stream_queue_buffer(m_pStream, pPwBuffer);

    /*if (!app::g_pWindow->bRunning) pw_main_loop_quit(pLoop);*/
    /* set bRunning for the mixer outside */
}

} /* namespace pipewire */
} /* namespace platform */
