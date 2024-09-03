#include "Mixer.hh"

#include "adt/logs.hh"
#include "adt/utils.hh"
#include "frame.hh"

#include <math.h>


namespace platform
{
namespace pipewire
{

[[maybe_unused]] constexpr f64 M_PI_M2 = M_PI + M_PI;

static void MixerRunThread(Mixer* s, int argc, char** argv);
static void cbOnProcess(void* data);
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
    .process = cbOnProcess,
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
}

void
MixerAdd(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (s->aTracks.size < MAX_TRACK_COUNT) ArrayPush(&s->aTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT(%u) reached, ignoring track push\n", MAX_TRACK_COUNT);

    mtx_unlock(&s->mtxAdd);
}

void
MixerAddBackground(Mixer* s, audio::Track t)
{
    mtx_lock(&s->mtxAdd);

    if (s->aTracks.size < MAX_TRACK_COUNT) ArrayPush(&s->aBackgroundTracks, t);
    else LOG_WARN("MAX_TRACK_COUNT(%u) reached, ignoring track push\n", MAX_TRACK_COUNT);

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

    COUT("sampleRate: %u\n", s->sampleRate);
    COUT("channels: %u\n", s->channels);

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

static void
cbOnProcess(void* data)
{
    auto* s = (Mixer*)data;

    pw_buffer* b;
    if ((b = pw_stream_dequeue_buffer(s->pStream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    spa_buffer* buf = b->buffer;
    s16* dst = (s16*)buf->datas[0].data;

    if (!dst)
    {
        LOG_WARN("dst == nullptr\n");
        return;
    }

    u32 stride = sizeof(f32) * s->channels;
    u32 nFrames = buf->datas[0].maxsize / stride;
    if (b->requested) nFrames = SPA_MIN(b->requested, (u64)nFrames);

    if (nFrames > 1024*4) nFrames = 1024*4; /* limit to arbitrary number */

    s->lastNFrames = nFrames;

    for (u32 i = 0; i < nFrames; i++)
    {
        s16 val[4] {};

        auto procTrack = [&val, s](audio::Track* track, u32* i) -> void {
            auto& t = *track;
            f32 vol = powf(t.volume, 3.0f);

            if (t.pcmPos + 4 < t.pcmSize)
            {
                /* FIXME: why 4 times, should be 2? */
                val[0] += t.pData[t.pcmPos + 0] * vol;
                val[1] += t.pData[t.pcmPos + 1] * vol;
                val[2] += t.pData[t.pcmPos + 2] * vol;
                val[3] += t.pData[t.pcmPos + 3] * vol;
                t.pcmPos += 4;
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
                    ArrayPopAsLast(&s->aTracks, *i);
                    --(*i);
                    mtx_unlock(&s->mtxAdd);
                }
            }
        };

        if (s->aBackgroundTracks.size > 0)
        {
            auto& t = s->aBackgroundTracks[s->currentBackgroundTrackIdx];
            f32 vol = powf(t.volume, 3.0f);

            if (t.pcmPos + 4 < t.pcmSize)
            {
                val[0] += t.pData[t.pcmPos + 0] * vol;
                val[1] += t.pData[t.pcmPos + 1] * vol;
                val[2] += t.pData[t.pcmPos + 2] * vol;
                val[3] += t.pData[t.pcmPos + 3] * vol;
                t.pcmPos += 4;
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
            procTrack(&t, &i);
        }

        *dst++ = val[0];
        *dst++ = val[1];
        *dst++ = val[2];
        *dst++ = val[3];
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    pw_stream_queue_buffer(s->pStream, b);

    if (!frame::g_pApp->bRunning) pw_main_loop_quit(s->pLoop);
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
