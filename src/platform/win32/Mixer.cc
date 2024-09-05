#include "Mixer.hh"
#include "adt/logs.hh"

namespace platform
{
namespace win32
{

static void MixerRunThread(Mixer* s, int argc, char** argv);

class XAudio2Voice : public IXAudio2VoiceCallback
{
public:
    IXAudio2SourceVoice* m_pVoice;
    audio::Track track;

    virtual void OnVoiceProcessingPassStart(UINT32 bytesRequired) noexcept override;
    virtual void OnVoiceProcessingPassEnd() noexcept override;
    virtual void OnStreamEnd() noexcept override;
    virtual void OnBufferStart(void* pBufferContext) noexcept override;
    virtual void OnBufferEnd(void* pBufferContext) noexcept override;
    virtual void OnLoopEnd(void* pBufferContext) noexcept override;
    virtual void OnVoiceError(void* pBufferContext, HRESULT error) noexcept override;
};

/*static XAudio2Voice voiceArr[audio::MAX_TRACK_COUNT * 2];*/
static XAudio2Voice s_xVoice {};
static s16 s_chunk[audio::CHUNK_SIZE] {};

void
MixerInit(Mixer* s, int argc, char** argv)
{
    s->base.bRunning = true;
    s->base.bMuted = false;
    s->base.volume = 0.1f;

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

    /*thrd_create(&s->threadLoop, fnp, &a);*/
    /*thrd_detach(s->threadLoop);*/

    MixerRunThread(s, argc, argv);
}

void
MixerDestroy([[maybe_unused]] Mixer* s)
{
    //
}

void
MixerAdd(Mixer* s, audio::Track t)
{
    //
}

void
MixerAddBackground(Mixer* s, audio::Track t)
{
    XAUDIO2_BUFFER b {};
    b.Flags = XAUDIO2_END_OF_STREAM;
    b.AudioBytes = t.pcmSize * 2;
    b.pAudioData = (BYTE*)t.pData;
    b.LoopCount = XAUDIO2_LOOP_INFINITE;

    COUT("sent: size(%u), ptr(%x)\n", t.pcmSize * 2, t.pData);

    HRESULT hr = s_xVoice.m_pVoice->SubmitSourceBuffer(&b);
    if (FAILED(hr)) LOG_WARN("SubmitSourceBuffer: failed\n");
}

static void
MixerRunThread(Mixer* s, int argc, char** argv)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    assert(!FAILED(hr));

    hr = XAudio2Create(&s->pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(!FAILED(hr));

    IXAudio2MasteringVoice* masterVoice = nullptr;
    hr = s->pXAudio2->CreateMasteringVoice(&masterVoice);
    assert(!FAILED(hr));

    WAVEFORMATEX wave {};
    wave.wFormatTag = WAVE_FORMAT_PCM;
    wave.nChannels = 2;
    wave.nSamplesPerSec = 48000;
    wave.wBitsPerSample = 16;
    wave.nBlockAlign = (2 * wave.wBitsPerSample) / 8;
    wave.nAvgBytesPerSec = wave.nSamplesPerSec * wave.nBlockAlign;

    hr = s->pXAudio2->CreateSourceVoice(
        &s_xVoice.m_pVoice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &s_xVoice, {}, {}
    );
    assert(!FAILED(hr));


    /*s_xVoice.m_pVoice->SetVolume(1.0f);*/
    s_xVoice.m_pVoice->Start();
}

void
XAudio2Voice::OnVoiceProcessingPassStart(UINT32 bytesRequired) noexcept
{
}

void
XAudio2Voice::OnVoiceProcessingPassEnd() noexcept
{
}

void
XAudio2Voice::OnStreamEnd() noexcept
{
}

void
XAudio2Voice::OnBufferStart(void* pBufferContext) noexcept
{
}

void
XAudio2Voice::OnBufferEnd(void* pBufferContext) noexcept
{
}

void
XAudio2Voice::OnLoopEnd(void* pBufferContext) noexcept
{
}

void
XAudio2Voice::OnVoiceError(void* pBufferContext, HRESULT error) noexcept
{
}

} /* namespace win32 */
} /* namespace platform */
