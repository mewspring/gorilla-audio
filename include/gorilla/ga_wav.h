#ifndef _GORILLA_GA_WAV_H
#define _GORILLA_GA_WAV_H

#include "ga.h"
#include "ga_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct ga_WavData
{
  ga_int32 fileSize;
  ga_int16 fmtTag, channels, blockAlign, bitsPerSample;
  ga_int32 fmtSize, sampleRate, bytesPerSec;
  ga_int32 dataOffset, dataSize;
} ga_WavData;

ga_result gaX_sound_load_wav_header(const char* in_filename, ga_uint32 in_byteOffset, ga_WavData* out_wavData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_WAV_H */
