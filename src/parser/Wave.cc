#include "Wave.hh"
#include "adt/logs.hh"

namespace parser
{

/* http://soundfile.sapp.org/doc/WaveFormat/ */

void
WaveParse(Wave* s)
{
    BinSetPos(&s->bin, 0);

    String chunkID = BinReadString(&s->bin, 4);
    DCOUT("chunkID: '%.*s'\n", chunkID.size, chunkID.pData);
    assert(chunkID == "RIFF" && "not a Wave header");

    u32 chunkSize = BinRead32(&s->bin);
    DCOUT("chunkSize: %u\n", chunkSize);

    String format = BinReadString(&s->bin, 4);
    DCOUT("format: '%.*s'\n", format.size, format.pData);
    assert(format == "WAVE" && "not a Wave header");

    String subchunk1ID = BinReadString(&s->bin, 4);
    DCOUT("subchunk1ID: '%.*s'\n", subchunk1ID.size, subchunk1ID.pData);
    assert(subchunk1ID == "fmt " && "not a Wave header");

    u32 subchunk1Size = BinRead32(&s->bin);
    DCOUT("subchunk1Size: %u\n", subchunk1Size);

    s16 audioFormat = BinRead16(&s->bin);
    DCOUT("audioFormat: %d\n", audioFormat);

    s16 numChannels = BinRead16(&s->bin);
    DCOUT("numChannels: %d\n", numChannels);

    u32 sampleRate = BinRead32(&s->bin);
    DCOUT("sampleRate: %u\n", sampleRate);

    u32 byteRate = BinRead32(&s->bin); /* == SampleRate * NumChannels * BitsPerSample/8 */
    DCOUT("byteRate: %u\n", byteRate);

    u16 blockAlign = BinRead16(&s->bin); /*  == NumChannels * BitsPerSample/8
                                          *  The number of bytes for one sample including
                                          *  all channels. I wonder what happens when
                                          *  this number isn't an integer? */
    DCOUT("blockAlign: %u\n", blockAlign);

    u16 bitsPerSample = BinRead16(&s->bin);
    DCOUT("bitsPerSample: %u\n", bitsPerSample);

    String subchunk2ID = BinReadString(&s->bin, 4);
    DCOUT("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);

    if (subchunk2ID == "LIST")
    {
        /* naively skip metadata */
        String data {};
        while (data != "data")
        {
            data = BinReadString(&s->bin, 4);
            s->bin.pos -= 3;
        }
        s->bin.pos -= 1;

        String list = BinReadString(&s->bin, 4);
        DCOUT("list: '%.*s'\n", list.size, list.pData);
    }
    else if (subchunk2ID == "data")
    {
        DCOUT("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);
    }

    u32 subchunk2Size = BinRead32(&s->bin); /* == NumSamples * NumChannels * BitsPerSample/8
                                             * This is the number of bytes in the data.
                                             * You can also think of this as the size
                                             * of the read of the subchunk following this 
                                             * number. */
    DCOUT("subchunk2Size: %u\n", subchunk2Size);

    s->sampleRate = sampleRate;
    s->nChannels = numChannels;
    s->pPcmData = reinterpret_cast<s16*>(&s->bin.sFile[s->bin.pos]);
    DCOUT("fileSize: %u, pos: %u\n\n", s->bin.sFile.size, s->bin.pos);
    s->pcmSize = subchunk2Size;
}

} /* namespace parser */
