#include "gorilla/ga_wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

gc_result gaX_sound_load_wav_header(const char* in_filename, gc_uint32 in_byteOffset, ga_WavData* out_wavData)
{
  char id[5];
  ga_WavData* wavData = out_wavData;
  FILE* f = fopen(in_filename, "rb");
  if(!f)
    return GC_ERROR_GENERIC;
  if(in_byteOffset)
    fseek(f, SEEK_SET, in_byteOffset);
  fread(&id[0], sizeof(char), 4, f); /* 'RIFF' */
  id[4] = 0;
  if(!strcmp(id, "RIFF"))
  {
    fread(&wavData->fileSize, sizeof(gc_int32), 1, f);
    fread(&id[0], sizeof(char), 4, f); /* 'WAVE' */
    if(!strcmp(id, "WAVE"))
    {
      gc_int32 dataFound;
      gc_int32 fmtStart;
      fread(&id[0], sizeof(char), 4, f); /* 'fmt ' */
      fread(&wavData->fmtSize, sizeof(gc_int32), 1, f);
      fmtStart = ftell(f);
      fread(&wavData->fmtTag, sizeof(gc_int16), 1, f);
      fread(&wavData->channels, sizeof(gc_int16), 1, f);
      fread(&wavData->sampleRate, sizeof(gc_int32), 1, f);
      fread(&wavData->bytesPerSec, sizeof(gc_int32), 1, f);
      fread(&wavData->blockAlign, sizeof(gc_int16), 1, f);
      fread(&wavData->bitsPerSample, sizeof(gc_int16), 1, f);
      fseek(f, fmtStart + wavData->fmtSize, SEEK_SET);
      do
      {
        fread(&id[0], sizeof(char), 4, f); /* 'data' */
        fread(&wavData->dataSize, sizeof(gc_int32), 1, f);
        dataFound = !strcmp(id, "data");
        if(dataFound)
          wavData->dataOffset = ftell(f);
        else
          fseek(f, wavData->dataSize, SEEK_CUR);
      } while(!dataFound && !feof(f));
      fclose(f);
      f = 0;
      if(dataFound)
        return GC_SUCCESS;
    }
  }
  if(f)
    fclose(f);
  return GC_ERROR_GENERIC;
}
