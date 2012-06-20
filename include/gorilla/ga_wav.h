#ifndef _GORILLA_GA_WAV_H
#define _GORILLA_GA_WAV_H

#include "ga.h"
#include "common/gc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct ga_WavData
{
  gc_int32 fileSize;
  gc_int16 fmtTag, channels, blockAlign, bitsPerSample;
  gc_int32 fmtSize, sampleRate, bytesPerSec;
  gc_int32 dataOffset, dataSize;
} ga_WavData;

gc_result gaX_sound_load_wav_header(const char* in_filename, gc_uint32 in_byteOffset, ga_WavData* out_wavData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_WAV_H */
