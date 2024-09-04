#include "Mixer.hh"

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

static XAudio2Voice voiceArr[audio::MAX_TRACK_COUNT * 2];

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

    thrd_create(&s->threadLoop, fnp, &a);
    thrd_detach(s->threadLoop);
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

    for (auto& v: voiceArr)
    {
        hr = s->pXAudio2->CreateSourceVoice(
            &v.m_pVoice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &v, {}, {}
        );
        v.m_pVoice->SetVolume(1.0f);
    }
}

void
XAudio2Voice::OnVoiceProcessingPassStart(UINT32 bytesRequired) noexcept
{
    XAUDIO2_BUFFER buff {};
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
