#include "Mixer.hh"

namespace platform
{
namespace win32
{

static void MixerRunThread(Mixer* s, int argc, char** argv);

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

    hr = s->pXAudio2->CreateSourceVoice(&s->pSourceVoice, &wave);
    assert(!FAILED(hr));

    XAUDIO2_BUFFER buff {};
}

} /* namespace win32 */
} /* namespace platform */
