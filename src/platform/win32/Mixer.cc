#include "Mixer.hh"
#include "adt/logs.hh"

#include <cmath>

namespace platform
{
namespace win32
{

static void MixerRunThread(Mixer* s);

class XAudio2VoiceInterface : public IXAudio2VoiceCallback
{
public:
    IXAudio2SourceVoice* m_pVoice = nullptr;
    bool m_bPlaying = false;
    audio::Track m_track {};

    virtual void OnVoiceProcessingPassStart(UINT32 bytesRequired) noexcept override;
    virtual void OnVoiceProcessingPassEnd() noexcept override;
    virtual void OnStreamEnd() noexcept override;
    virtual void OnBufferStart(void* pBufferContext) noexcept override;
    virtual void OnBufferEnd(void* pBufferContext) noexcept override;
    virtual void OnLoopEnd(void* pBufferContext) noexcept override;
    virtual void OnVoiceError(void* pBufferContext, HRESULT error) noexcept override;
};

static XAudio2VoiceInterface s_aVoices[audio::MAX_TRACK_COUNT];
/*static XAudio2Voice s_xVoice {};*/
/*static s16 s_chunk[audio::CHUNK_SIZE] {};*/

static const audio::MixerVTable sc_XAudio2MixerVTable {
    .start = decltype(audio::MixerVTable::start)(MixerStart),
    .destroy = decltype(audio::MixerVTable::destroy)(MixerDestroy),
    .add = decltype(audio::MixerVTable::add)(MixerAdd),
    .addBackground = decltype(audio::MixerVTable::addBackground)(MixerAddBackground),
};

Mixer::Mixer(IAllocator* pA)
    : super {&sc_XAudio2MixerVTable},
      aTracks(pA, audio::MAX_TRACK_COUNT),
      aBackgroundTracks(pA, audio::MAX_TRACK_COUNT)
{
    this->super.bRunning = true;
    this->super.bMuted = false;
    this->super.volume = 0.1f;

    mtx_init(&this->mtxAdd, mtx_plain);
}

void
MixerStart(Mixer* s)
{
    MixerRunThread(s);
}

void
MixerDestroy([[maybe_unused]] Mixer* s)
{
    //
}

void
MixerAdd(Mixer* s, audio::Track t)
{
    /* find free slot */
    for (auto& v : s_aVoices)
    {
        if (!v.m_bPlaying)
        {
            XAUDIO2_BUFFER b {
                .Flags = XAUDIO2_END_OF_STREAM,
                .AudioBytes = t.pcmSize * 2,
                .pAudioData = (BYTE*)t.pData,
                .PlayBegin = 0,
                .PlayLength = 0,
                .LoopBegin = 0,
                .LoopLength = 0,
                .LoopCount = 0,
                .pContext = s
            };

            auto hr = v.m_pVoice->SubmitSourceBuffer(&b);
            if (FAILED(hr)) LOG_WARN("SubmitSourceBuffer: failed\n");

            v.m_pVoice->SetVolume(std::pow(t.volume, 3.0f));
            v.m_bPlaying = true;
            break;
        }
    }
}

void
MixerAddBackground(Mixer* s, audio::Track t)
{
    /* find free slot */
    for (auto& v : s_aVoices)
    {
        if (!v.m_bPlaying)
        {
            XAUDIO2_BUFFER b {
                .Flags = XAUDIO2_END_OF_STREAM,
                .AudioBytes = t.pcmSize * 2,
                .pAudioData = (BYTE*)t.pData,
                .PlayBegin = 0,
                .PlayLength = 0,
                .LoopBegin = 0,
                .LoopLength = 0,
                .LoopCount = 0,
                .pContext = nullptr
            };

            auto hr = v.m_pVoice->SubmitSourceBuffer(&b);
            if (FAILED(hr)) LOG_WARN("SubmitSourceBuffer: failed\n");

            v.m_pVoice->SetVolume(std::pow(t.volume, 3.0f));
            v.m_bPlaying = true;
            v.m_track = t;
            break;
        }
    }
}

static void
MixerRunThread(Mixer* s)
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

    for (auto& v : s_aVoices)
    {
        hr = s->pXAudio2->CreateSourceVoice(
            &v.m_pVoice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &v, {}, {}
        );

        assert(!FAILED(hr));

        v.m_pVoice->SetVolume(audio::g_globalVolume);
        v.m_pVoice->Start();
        v.m_bPlaying = false;
    }
}

void
XAudio2VoiceInterface::OnVoiceProcessingPassStart(UINT32 bytesRequired) noexcept
{
    /*COUT("OnVoiceProcessingPassStart: bytesRequired: %u\n", bytesRequired);*/
}

void
XAudio2VoiceInterface::OnVoiceProcessingPassEnd() noexcept
{
    /*COUT("OnVoiceProcessingPassEnd\n");*/
}

void
XAudio2VoiceInterface::OnStreamEnd() noexcept
{
    m_pVoice->FlushSourceBuffers();

    if (m_track.bRepeat)
    {
        auto& t = m_track;

        XAUDIO2_BUFFER b {
            .Flags = XAUDIO2_END_OF_STREAM,
            .AudioBytes = t.pcmSize * 2,
            .pAudioData = (BYTE*)t.pData,
            .PlayBegin = 0,
            .PlayLength = 0,
            .LoopBegin = 0,
            .LoopLength = 0,
            .LoopCount = 0,
            .pContext = nullptr
        };

        auto hr = m_pVoice->SubmitSourceBuffer(&b);
        if (FAILED(hr)) LOG_WARN("SubmitSourceBuffer: failed\n");

        m_pVoice->SetVolume(t.volume);
        m_bPlaying = true;
        m_track = t;
    }
    else m_bPlaying = false;
}

void
XAudio2VoiceInterface::OnBufferStart(void* pBufferContext) noexcept
{
    /*COUT("OnBufferStart\n");*/
}

void
XAudio2VoiceInterface::OnBufferEnd(void* pBufferContext) noexcept
{
    auto* s = (Mixer*)pBufferContext;
}

void
XAudio2VoiceInterface::OnLoopEnd(void* pBufferContext) noexcept
{
    /*COUT("OnLoopEnd\n");*/
}

void
XAudio2VoiceInterface::OnVoiceError(void* pBufferContext, HRESULT error) noexcept
{
    LOG_WARN("OnVoiceError\n");
}

} /* namespace win32 */
} /* namespace platform */
