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

struct ThrdLoopLockGuard
{
    pw_thread_loop* p;

    ThrdLoopLockGuard() = delete;
    ThrdLoopLockGuard(pw_thread_loop* _p) : p(_p) { pw_thread_loop_lock(_p); }
    ~ThrdLoopLockGuard() { pw_thread_loop_unlock(p); }
};

static void MixerRunThread(Mixer* s, int argc, char** argv);
static void onProcess(void* data);
static bool MixerEmpty(Mixer* s);

static const pw_stream_events s_streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy {},
    .state_changed {},
    .control_info {},
    .io_changed {},
    .param_changed {},
    .add_buffer {},
    .remove_buffer {},
    .process = onProcess,
    .drained {},
    .command {},
    .trigger_done {},
};

static const audio::MixerVTable sc_MixerVTable {
    .start = decltype(audio::MixerVTable::start)(MixerStart),
    .destroy = decltype(audio::MixerVTable::destroy)(MixerDestroy),
    .add = decltype(audio::MixerVTable::add)(MixerAdd),
    .addBackground = decltype(audio::MixerVTable::addBackground)(MixerAddBackground),
};

Mixer::Mixer(IAllocator* pA)
    : super(&sc_MixerVTable),
      aTracks(pA, audio::MAX_TRACK_COUNT),
      aBackgroundTracks(pA, audio::MAX_TRACK_COUNT)
{
    this->super.bRunning = true;
    this->super.bMuted = false;
    this->super.volume = 0.1f;

    this->sampleRate = 48000;
    this->channels = 2;
    this->eformat = SPA_AUDIO_FORMAT_S16_LE;

    mtx_init(&this->mtxAdd, mtx_plain);
}

void
MixerStart(Mixer* s)
{
    MixerRunThread(s, app::g_argc, app::g_argv);
}

void
MixerDestroy(Mixer* s)
{
    {
        ThrdLoopLockGuard tLock(s->pThrdLoop);
        pw_stream_set_active(s->pStream, true);
    }

    pw_thread_loop_stop(s->pThrdLoop);
    LOG_NOTIFY("pw_thread_loop_stop()\n");

    s->super.bRunning = false;

    pw_stream_destroy(s->pStream);
    pw_thread_loop_destroy(s->pThrdLoop);
    pw_deinit();

    VecDestroy(&s->aTracks);
    VecDestroy(&s->aBackgroundTracks);
    mtx_destroy(&s->mtxAdd);
}

void
MixerAdd(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (VecSize(&s->aTracks) < audio::MAX_TRACK_COUNT) VecPush(&s->aTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT({}) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&s->mtxAdd);
}

void
MixerAddBackground(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (VecSize(&s->aTracks) < audio::MAX_TRACK_COUNT) VecPush(&s->aBackgroundTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT({}) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&s->mtxAdd);
}

static void
MixerRunThread(Mixer* s, int argc, char** argv)
{
    pw_init(&argc, &argv);

    u8 aBuff[1024] {};
    const spa_pod* aParams[1] {};
    spa_pod_builder b {};
    spa_pod_builder_init(&b, aBuff, sizeof(aBuff));

    s->pThrdLoop = pw_thread_loop_new("BreakoutThreadLoop", {});

    s->pStream = pw_stream_new_simple(
        pw_thread_loop_get_loop(s->pThrdLoop),
        "BreakoutAudioSource",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Game",
            nullptr
        ),
        &s_streamEvents,
        s
    );

    spa_audio_info_raw rawInfo {
        .format = s->eformat,
        .flags {},
        .rate = s->sampleRate,
        .channels = s->channels,
        .position {}
    };

    aParams[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &rawInfo);

    pw_stream_connect(
        s->pStream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT|PW_STREAM_FLAG_INACTIVE|PW_STREAM_FLAG_MAP_BUFFERS),
        aParams,
        utils::size(aParams)
    );

    pw_thread_loop_start(s->pThrdLoop);

    ThrdLoopLockGuard tLock(s->pThrdLoop);
    pw_stream_set_active(s->pStream, true);
}

//__attribute__((target("default")))
static void
writeFrames(Mixer* s, void* pBuff, u32 nFrames)
{
    __m128i_u* pSimdDest = (__m128i_u*)pBuff;

    for (u32 i = 0; i < nFrames / 4; i++)
    {
        __m128i packed8Samples {};

        if (VecSize(&s->aBackgroundTracks) > 0)
        {
            auto& t = s->aBackgroundTracks[s->currentBackgroundTrackIdx];
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
                auto current = s->currentBackgroundTrackIdx + 1;
                if (current > VecSize(&s->aBackgroundTracks) - 1) current = 0;
                s->currentBackgroundTrackIdx = current;
            }
        }

        for (u32 i = 0; i < VecSize(&s->aTracks); i++)
        {
            auto& t = s->aTracks[i];
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
                    mtx_lock(&s->mtxAdd);
                    VecPopAsLast(&s->aTracks, i);
                    --i;
                    mtx_unlock(&s->mtxAdd);
                }
            }
        }

        _mm_storeu_si128(pSimdDest, packed8Samples);

        pSimdDest++;
    }
}

//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wunused-function"
//
//__attribute__((target("avx2")))
//static void
//writeFrames(Mixer* s, void* pBuff, u32 nFrames)
//{
//    __m256i_u* pSimdDest = (__m256i_u*)pBuff;
//
//    for (u32 i = 0; i < nFrames / 8; i++)
//    {
//        __m256i packed16Samples {};
//
//        if (VecSize(&s->aBackgroundTracks) > 0)
//        {
//            auto& t = s->aBackgroundTracks[s->currentBackgroundTrackIdx];
//            f32 vol = powf(t.volume * audio::g_globalVolume, 3.0f);
//
//            if (t.pcmPos + 16 <= t.pcmSize)
//            {
//                auto what = _mm256_set_epi16(
//                    t.pData[t.pcmPos + 15] * vol,
//                    t.pData[t.pcmPos + 14] * vol,
//                    t.pData[t.pcmPos + 13] * vol,
//                    t.pData[t.pcmPos + 12] * vol,
//                    t.pData[t.pcmPos + 11] * vol,
//                    t.pData[t.pcmPos + 10] * vol,
//                    t.pData[t.pcmPos + 9 ] * vol,
//                    t.pData[t.pcmPos + 8 ] * vol,
//                    t.pData[t.pcmPos + 7 ] * vol,
//                    t.pData[t.pcmPos + 6 ] * vol,
//                    t.pData[t.pcmPos + 5 ] * vol,
//                    t.pData[t.pcmPos + 4 ] * vol,
//                    t.pData[t.pcmPos + 3 ] * vol,
//                    t.pData[t.pcmPos + 2 ] * vol,
//                    t.pData[t.pcmPos + 1 ] * vol,
//                    t.pData[t.pcmPos + 0 ] * vol
//                );
//
//                packed16Samples = _mm256_add_epi16(packed16Samples, what);
//
//                t.pcmPos += 16;
//            }
//            else
//            {
//                t.pcmPos = 0;
//                auto current = s->currentBackgroundTrackIdx + 1;
//                if (current > VecSize(&s->aBackgroundTracks) - 1) current = 0;
//                s->currentBackgroundTrackIdx = current;
//            }
//        }
//
//        for (u32 i = 0; i < VecSize(&s->aTracks); i++)
//        {
//            auto& t = s->aTracks[i];
//            f32 vol = powf(t.volume, 3.0f);
//
//            if (t.pcmPos + 16 <= t.pcmSize)
//            {
//                auto what = _mm256_set_epi16(
//                    t.pData[t.pcmPos + 15] * vol,
//                    t.pData[t.pcmPos + 14] * vol,
//                    t.pData[t.pcmPos + 13] * vol,
//                    t.pData[t.pcmPos + 12] * vol,
//                    t.pData[t.pcmPos + 11] * vol,
//                    t.pData[t.pcmPos + 10] * vol,
//                    t.pData[t.pcmPos + 9 ] * vol,
//                    t.pData[t.pcmPos + 8 ] * vol,
//                    t.pData[t.pcmPos + 7 ] * vol,
//                    t.pData[t.pcmPos + 6 ] * vol,
//                    t.pData[t.pcmPos + 5 ] * vol,
//                    t.pData[t.pcmPos + 4 ] * vol,
//                    t.pData[t.pcmPos + 3 ] * vol,
//                    t.pData[t.pcmPos + 2 ] * vol,
//                    t.pData[t.pcmPos + 1 ] * vol,
//                    t.pData[t.pcmPos + 0 ] * vol
//                );
//
//                packed16Samples = _mm256_add_epi16(packed16Samples, what);
//
//                t.pcmPos += 16;
//            }
//            else
//            {
//                if (t.bRepeat)
//                {
//                    t.pcmPos = 0;
//                }
//                else
//                {
//                    mtx_lock(&s->mtxAdd);
//                    VecPopAsLast(&s->aTracks, i);
//                    --i;
//                    mtx_unlock(&s->mtxAdd);
//                }
//            }
//        }
//
//        _mm256_storeu_si256(pSimdDest, packed16Samples);
//
//        pSimdDest++;
//    }
//}
//
//#pragma clang diagnostic pop

static void
onProcess(void* data)
{
    auto* s = (Mixer*)data;

    pw_buffer* pPwBuffer = pw_stream_dequeue_buffer(s->pStream);
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

    u32 stride = sizeof(s16) * s->channels;
    u32 nFrames = pBuffData.maxsize / stride;
    if (pPwBuffer->requested) nFrames = SPA_MIN(pPwBuffer->requested, (u64)nFrames);

    if (nFrames > 1024*4) nFrames = 1024*4; /* limit to arbitrary number */

    s->lastNFrames = nFrames;

    writeFrames(s, pDest, nFrames);

    pBuffData.chunk->offset = 0;
    pBuffData.chunk->stride = stride;
    pBuffData.chunk->size = nFrames * stride;

    pw_stream_queue_buffer(s->pStream, pPwBuffer);

    /*if (!app::g_pWindow->bRunning) pw_main_loop_quit(s->pLoop);*/
    /* set bRunning for the mixer outside */
}

[[maybe_unused]] static bool
MixerEmpty(Mixer* s)
{
    mtx_lock(&s->mtxAdd);
    bool r = VecSize(&s->aTracks) > 0;
    mtx_unlock(&s->mtxAdd);

    return r;
}

} /* namespace pipewire */
} /* namespace platform */
