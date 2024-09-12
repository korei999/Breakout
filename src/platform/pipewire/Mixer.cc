#include "Mixer.hh"

#include "adt/logs.hh"
#include "adt/utils.hh"
#include "app.hh"

#include <emmintrin.h>
#include <immintrin.h>
#include <math.h>

namespace platform
{
namespace pipewire
{

static void MixerRunThread(Mixer* s, int argc, char** argv);
static void onProcess(void* data);
static bool MixerEmpty(Mixer* s);

const pw_stream_events Mixer::s_streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy {},
    .state_changed = {},
    .control_info {},
    .io_changed = {},
    .param_changed = {},
    .add_buffer {},
    .remove_buffer {},
    .process = onProcess,
    .drained {},
    .command {},
    .trigger_done {},
};

void
MixerInit(Mixer* s, int argc, char** argv)
{
    s->base.bRunning = true;
    s->base.bMuted = false;
    s->base.volume = 0.1f;

    s->sampleRate = 48000;
    s->channels = 2;
    s->eformat = SPA_AUDIO_FORMAT_S16_LE;

    mtx_init(&s->mtxAdd, mtx_plain);

    struct Args
    {
        Mixer* s;
        int argc;
        char** argv;
    };

    static Args a {
        .s = s,
        .argc = argc,
        .argv = argv
    };

    auto fnp = +[](void* arg) -> int {
        auto a = *(Args*)arg;
        MixerRunThread(a.s, a.argc, a.argv);

        return thrd_success;
    };

    thrd_create(&s->threadLoop, fnp, &a);
    thrd_detach(s->threadLoop);
}

void
MixerDestroy(Mixer* s)
{
    mtx_destroy(&s->mtxAdd);
    VecDestroy(&s->aTracks);
    VecDestroy(&s->aBackgroundTracks);
}

void
MixerAdd(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (s->aTracks.size < audio::MAX_TRACK_COUNT) VecPush(&s->aTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT(%u) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&s->mtxAdd);
}

void
MixerAddBackground(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (s->aTracks.size < audio::MAX_TRACK_COUNT) VecPush(&s->aBackgroundTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT(%u) reached, ignoring track push\n", audio::MAX_TRACK_COUNT);

    mtx_unlock(&s->mtxAdd);
}

static void
MixerRunThread(Mixer* s, int argc, char** argv)
{
    pw_init(&argc, &argv);

    u8 setupBuffer[1024] {};
    const spa_pod* params[1] {};
    spa_pod_builder b = SPA_POD_BUILDER_INIT(setupBuffer, sizeof(setupBuffer));

    s->pLoop = pw_main_loop_new(nullptr);

    s->pStream = pw_stream_new_simple(
        pw_main_loop_get_loop(s->pLoop),
        "BreakoutAudioSource",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            nullptr
        ),
        &s->s_streamEvents,
        s
    );

    spa_audio_info_raw rawInfo {
        .format = s->eformat,
        .flags {},
        .rate = s->sampleRate,
        .channels = s->channels,
        .position {}
    };

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &rawInfo);

    pw_stream_connect(
        s->pStream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_ASYNC),
        params,
        utils::size(params)
    );

    pw_main_loop_run(s->pLoop);

    pw_stream_destroy(s->pStream);
    pw_main_loop_destroy(s->pLoop);
    pw_deinit();
    s->base.bRunning = false;
}

__attribute__((target("default")))
static void
writeFrames(Mixer* s, void* pBuff, u32 nFrames)
{
    __m128i_u* pSimdDest = (__m128i_u*)pBuff;

    for (u32 i = 0; i < nFrames / 4; i++)
    {
        __m128i packed8Samples {};

        if (s->aBackgroundTracks.size > 0)
        {
            auto& t = s->aBackgroundTracks[s->currentBackgroundTrackIdx];
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
                t.pcmPos = 0;
                auto current = s->currentBackgroundTrackIdx + 1;
                if (current > s->aBackgroundTracks.size - 1) current = 0;
                s->currentBackgroundTrackIdx = current;
            }
        }

        for (u32 i = 0; i < s->aTracks.size; i++)
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

__attribute__((target("avx2")))
static void
writeFrames(Mixer* s, void* pBuff, u32 nFrames)
{
    __m256i_u* pSimdDest = (__m256i_u*)pBuff;

    for (u32 i = 0; i < nFrames / 8; i++)
    {
        __m256i packed16Samples {};

        if (s->aBackgroundTracks.size > 0)
        {
            auto& t = s->aBackgroundTracks[s->currentBackgroundTrackIdx];
            f32 vol = powf(t.volume, 3.0f);

            if (t.pcmPos + 16 <= t.pcmSize)
            {
                auto what = _mm256_set_epi16(
                    t.pData[t.pcmPos + 15] * vol,
                    t.pData[t.pcmPos + 14] * vol,
                    t.pData[t.pcmPos + 13] * vol,
                    t.pData[t.pcmPos + 12] * vol,
                    t.pData[t.pcmPos + 11] * vol,
                    t.pData[t.pcmPos + 10] * vol,
                    t.pData[t.pcmPos + 9 ] * vol,
                    t.pData[t.pcmPos + 8 ] * vol,
                    t.pData[t.pcmPos + 7 ] * vol,
                    t.pData[t.pcmPos + 6 ] * vol,
                    t.pData[t.pcmPos + 5 ] * vol,
                    t.pData[t.pcmPos + 4 ] * vol,
                    t.pData[t.pcmPos + 3 ] * vol,
                    t.pData[t.pcmPos + 2 ] * vol,
                    t.pData[t.pcmPos + 1 ] * vol,
                    t.pData[t.pcmPos + 0 ] * vol
                );

                packed16Samples = _mm256_add_epi16(packed16Samples, what);

                t.pcmPos += 16;
            }
            else
            {
                t.pcmPos = 0;
                auto current = s->currentBackgroundTrackIdx + 1;
                if (current > s->aBackgroundTracks.size - 1) current = 0;
                s->currentBackgroundTrackIdx = current;
            }
        }

        for (u32 i = 0; i < s->aTracks.size; i++)
        {
            auto& t = s->aTracks[i];
            f32 vol = powf(t.volume, 3.0f);

            if (t.pcmPos + 16 <= t.pcmSize)
            {
                auto what = _mm256_set_epi16(
                    t.pData[t.pcmPos + 15] * vol,
                    t.pData[t.pcmPos + 14] * vol,
                    t.pData[t.pcmPos + 13] * vol,
                    t.pData[t.pcmPos + 12] * vol,
                    t.pData[t.pcmPos + 11] * vol,
                    t.pData[t.pcmPos + 10] * vol,
                    t.pData[t.pcmPos + 9 ] * vol,
                    t.pData[t.pcmPos + 8 ] * vol,
                    t.pData[t.pcmPos + 7 ] * vol,
                    t.pData[t.pcmPos + 6 ] * vol,
                    t.pData[t.pcmPos + 5 ] * vol,
                    t.pData[t.pcmPos + 4 ] * vol,
                    t.pData[t.pcmPos + 3 ] * vol,
                    t.pData[t.pcmPos + 2 ] * vol,
                    t.pData[t.pcmPos + 1 ] * vol,
                    t.pData[t.pcmPos + 0 ] * vol
                );

                packed16Samples = _mm256_add_epi16(packed16Samples, what);

                t.pcmPos += 16;
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

        _mm256_storeu_si256(pSimdDest, packed16Samples);

        pSimdDest++;
    }
}

#pragma clang diagnostic pop

static void
onProcess(void* data)
{
    auto* s = (Mixer*)data;

    pw_buffer* b;
    if ((b = pw_stream_dequeue_buffer(s->pStream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    auto pBuffData = b->buffer->datas[0];
    s16* pDest = (s16*)pBuffData.data;

    if (!pDest)
    {
        LOG_WARN("dst == nullptr\n");
        return;
    }

    u32 stride = sizeof(s16) * s->channels;
    u32 nFrames = pBuffData.maxsize / stride;
    if (b->requested) nFrames = SPA_MIN(b->requested, (u64)nFrames);

    if (nFrames > 1024*4) nFrames = 1024*4; /* limit to arbitrary number */

    s->lastNFrames = nFrames;

    writeFrames(s, pDest, nFrames);

    pBuffData.chunk->offset = 0;
    pBuffData.chunk->stride = stride;
    pBuffData.chunk->size = nFrames * stride;

    pw_stream_queue_buffer(s->pStream, b);

    if (!app::g_pApp->bRunning) pw_main_loop_quit(s->pLoop);
    /* set bRunning for the mixer outside */
}

[[maybe_unused]] static bool
MixerEmpty(Mixer* s)
{
    mtx_lock(&s->mtxAdd);
    bool r = s->aTracks.size > 0;
    mtx_unlock(&s->mtxAdd);

    return r;
}

} /* namespace pipewire */
} /* namespace platform */
