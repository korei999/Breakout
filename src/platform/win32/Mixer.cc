#include "Mixer.hh"
#include "adt/logs.hh"

#include <cmath>

namespace platform
{
namespace win32
{

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

Mixer::Mixer(IAllocator* pA)
    : m_mtxAdd(MUTEX_TYPE::PLAIN),
      m_aTracks(pA, audio::MAX_TRACK_COUNT),
      m_aBackgroundTracks(pA, audio::MAX_TRACK_COUNT)
{
    m_bRunning = true;
    m_bMuted = false;
    m_volume = 0.1f;
}

void
Mixer::start()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    assert(!FAILED(hr));

    hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(!FAILED(hr));

    IXAudio2MasteringVoice* masterVoice = nullptr;
    hr = m_pXAudio2->CreateMasteringVoice(&masterVoice);
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
        hr = m_pXAudio2->CreateSourceVoice(
            &v.m_pVoice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &v, {}, {}
        );

        assert(!FAILED(hr));

        v.m_pVoice->SetVolume(audio::g_globalVolume);
        v.m_pVoice->Start();
        v.m_bPlaying = false;
    }
}

void
Mixer::destroy()
{
    //
}

void
Mixer::add(audio::Track t)
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
                .pContext = this
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
Mixer::addBackground(audio::Track t)
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
