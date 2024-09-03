#include "Wave.hh"
#include "adt/logs.hh"

namespace parser
{

/* http://soundfile.sapp.org/doc/WaveFormat/ */

void
WaveParse(Wave* s)
{
    s->bin.pos = 0;

    String chunkID = BinReadString(&s->bin, 4);
    assert(chunkID == "RIFF" && "not a Wave header");

    u32 chunkSize = BinRead32(&s->bin);

    String format = BinReadString(&s->bin, 4);
    assert(format == "WAVE" && "not a Wave header");

    String subchunk1ID = BinReadString(&s->bin, 4);
    assert(subchunk1ID == "fmt " && "not a Wave header");

    u32 subchunk1Size = BinRead32(&s->bin);
    s16 audioFormat = BinRead16(&s->bin);
    s16 numChannels = BinRead16(&s->bin);
    u32 sampleRate = BinRead32(&s->bin);
    u32 byteRate = BinRead32(&s->bin); /* == SampleRate * NumChannels * BitsPerSample/8 */
    u16 blockAlign = BinRead16(&s->bin); /*  == NumChannels * BitsPerSample/8
                                          *  The number of bytes for one sample including
                                          *  all channels. I wonder what happens when
                                          *  this number isn't an integer? */

    u16 bitsPerSample = BinRead16(&s->bin);

    String subchunk2ID = BinReadString(&s->bin, 4);
    String data {};

    if (subchunk2ID == "LIST")
    {
        /* naively skip metadata */
        String _data {};
        while (_data != "data")
        {
            _data = BinReadString(&s->bin, 4);
            s->bin.pos -= 3;
        }
        s->bin.pos -= 1;

        data = BinReadString(&s->bin, 4);
    }
    else if (subchunk2ID == "data")
    {
    }

    u32 subchunk2Size = BinRead32(&s->bin); /* == NumSamples * NumChannels * BitsPerSample/8
                                             * This is the number of bytes in the data.
                                             * You can also think of this as the size
                                             * of the read of the subchunk following this 
                                             * number. */

    s->sampleRate = sampleRate;
    s->nChannels = numChannels;
    s->pPcmData = reinterpret_cast<s16*>(&s->bin.sFile[s->bin.pos]);
    s->pcmSize = subchunk2Size / sizeof(s16);

#ifdef WAVE
    LOG_OK("chunkID: '%.*s'\n", chunkID.size, chunkID.pData);
    LOG_OK("chunkSize: %u\n", chunkSize);
    LOG_OK("format: '%.*s'\n", format.size, format.pData);
    LOG_OK("subchunk1ID: '%.*s'\n", subchunk1ID.size, subchunk1ID.pData);
    LOG_OK("subchunk1Size: %u\n", subchunk1Size);
    LOG_OK("audioFormat: %d\n", audioFormat);
    LOG_OK("numChannels: %d\n", numChannels);
    LOG_OK("sampleRate: %u\n", sampleRate);
    LOG_OK("byteRate: %u\n", byteRate);
    LOG_OK("blockAlign: %u\n", blockAlign);
    LOG_OK("bitsPerSample: %u\n", bitsPerSample);
    LOG_OK("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);
    LOG_OK("data: '%.*s'\n", data.size, data.pData);
    LOG_OK("subchunk2Size: %u\n", subchunk2Size);
    LOG_OK("fileSize: %u, pos: %u\n", s->bin.sFile.size, s->bin.pos);
    CERR("\n");
#endif
}

} /* namespace parser */
