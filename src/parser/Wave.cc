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
    COUT("chunkID: '%.*s'\n", chunkID.size, chunkID.pData);
    assert(chunkID == "RIFF" && "not a Wave header");

    u32 chunkSize = BinRead32(&s->bin);
    COUT("chunkSize: %u\n", chunkSize);

    String format = BinReadString(&s->bin, 4);
    COUT("format: '%.*s'\n", format.size, format.pData);
    assert(format == "WAVE" && "not a Wave header");

    String subchunk1ID = BinReadString(&s->bin, 4);
    COUT("subchunk1ID: '%.*s'\n", subchunk1ID.size, subchunk1ID.pData);
    assert(subchunk1ID == "fmt " && "not a Wave header");

    u32 subchunk1Size = BinRead32(&s->bin);
    COUT("subchunk1Size: %u\n", subchunk1Size);

    s16 audioFormat = BinRead16(&s->bin);
    COUT("audioFormat: %d\n", audioFormat);

    s16 numChannels = BinRead16(&s->bin);
    COUT("numChannels: %d\n", numChannels);

    u32 sampleRate = BinRead32(&s->bin);
    COUT("sampleRate: %u\n", sampleRate);

    u32 byteRate = BinRead32(&s->bin); /* == SampleRate * NumChannels * BitsPerSample/8 */
    COUT("byteRate: %u\n", byteRate);

    u16 blockAlign = BinRead16(&s->bin); /*  == NumChannels * BitsPerSample/8
                                          *  The number of bytes for one sample including
                                          *  all channels. I wonder what happens when
                                          *  this number isn't an integer? */
    COUT("blockAlign: %u\n", blockAlign);

    u16 bitsPerSample = BinRead16(&s->bin);
    COUT("bitsPerSample: %u\n", bitsPerSample);

    /* skip unneeded metadata */
    // String subchunk2ID;
    // do
    // {
    //     subchunk2ID = BinReadString(&s->bin, 4);
    // } while (subchunk2ID != "data");

    // COUT("subchunk2ID: '%.*s'\n", subchunk2ID.size, subchunk2ID.pData);

    // u32 subchunk2Size = BinRead32(&s->bin); /* == NumSamples * NumChannels * BitsPerSample/8
    //                                          * This is the number of bytes in the data.
    //                                          * You can also think of this as the size
    //                                          * of the read of the subchunk following this 
    //                                          * number. */
    // COUT("subchunk2Size: %u\n", subchunk2Size);

    s->sampleRate = sampleRate;
    s->nChannels = numChannels;
    s->pPcmData = reinterpret_cast<s16*>(&s->bin.sFile[0]);
    COUT("fileSize: %u, pos: %u\n", s->bin.sFile.size, s->bin.pos);
    s->pcmSize = chunkSize;
}

} /* namespace parser */
