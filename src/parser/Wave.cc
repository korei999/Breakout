#include "Wave.hh"

#include "adt/logs.hh"

namespace parser
{

/* https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html */

void
WaveParse(Wave* s)
{
    [[maybe_unused]] String ckID = BinReadString(&s->bin, 4);
    assert(ckID == "RIFF" && "not a Wave header");

    [[maybe_unused]] u32 ckSize = BinRead32(&s->bin);

    [[maybe_unused]] String waveid = BinReadString(&s->bin, 4);
    assert(waveid == "WAVE" && "not a Wave header");

    /* FMT CHUNK */
    [[maybe_unused]] String fmtCkID = BinReadString(&s->bin, 4);
    assert(fmtCkID == "fmt " && "not a Wave header");

    [[maybe_unused]] u32 fmtCkSize = BinRead32(&s->bin);
    assert(fmtCkSize == 16 || fmtCkSize == 18 || fmtCkSize == 40);

    [[maybe_unused]] s16 wFormatTag = BinRead16(&s->bin);
    if (wFormatTag != 1 && wFormatTag != 3)
    {
        LOG_WARN("wFormatTag: '{:#x}' unsupported Wave format\n", wFormatTag);
        return;
    }

    s16 nChannels = BinRead16(&s->bin);
    u32 nSamplesPerSec = BinRead32(&s->bin);
    [[maybe_unused]] s32 nAvgBytesPerSec = BinRead32(&s->bin);
    [[maybe_unused]] s16 nBlockAlign = BinRead16(&s->bin);

    [[maybe_unused]] u16 wBitsPerSample = BinRead16(&s->bin);

    String subchunk2ID = BinReadString(&s->bin, 4);

    String data {};
    if (subchunk2ID != "data")
    {
        /* skip metadata */
        String __data {};
        while (__data != "data" && s->bin.pos + 4 < s->bin.sFile.size)
        {
            __data = BinReadString(&s->bin, 4);
            s->bin.pos -= 3;

        }
        s->bin.pos -= 1;

        data = BinReadString(&s->bin, 4);
    }
    else
    {
        /* TODO: qfact LIST  */
    }

    if (data != "data")
    {
        LOG_WARN("error loading wave file (no 'data' chunk)\n");
        return;
    }

    u32 subchunk2Size = BinRead32(&s->bin);

    s->sampleRate = nSamplesPerSec;
    s->nChannels = nChannels;
    s->pPcmData = reinterpret_cast<s16*>(&s->bin.sFile[s->bin.pos]);
    s->pcmSize = subchunk2Size / sizeof(s16);

#ifdef D_WAVE
    LOG_OK("'%.*s':\n", s->bin.sPath.size, s->bin.sPath.pData);
    LOG_OK("ckID: '%.*s'\n", ckID.size, ckID.pData);
    LOG_OK("ckSize: %u\n", ckSize);
    LOG_OK("waveid: '%.*s'\n", waveid.size, waveid.pData);
    LOG_OK("fmtCkID: '%.*s'\n", fmtCkID.size, fmtCkID.pData);
    LOG_OK("fmtCkSize: %u\n", fmtCkSize);
    LOG_OK("wFormatTag: %p\n", reinterpret_cast<void*>(wFormatTag));
    LOG_OK("nChannels: %d\n", nChannels);
    LOG_OK("nSamplesPerSec: %u\n", nSamplesPerSec);
    LOG_OK("nAvgBytesPerSec: %u\n", nAvgBytesPerSec);
    LOG_OK("nBlockAlign: %u\n", nBlockAlign);
    LOG_OK("wBitsPerSample: %u\n", wBitsPerSample);
    LOG_OK("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);
    LOG_OK("data: '%.*s'\n", data.size, data.pData);
    LOG_OK("subchunk2Size: %u\n", subchunk2Size);
    LOG_OK("fileSize: %u, pos: %u\n\n", s->bin.sFile.size, s->bin.pos);
#endif
}

} /* namespace parser */
