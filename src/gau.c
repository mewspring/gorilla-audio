#include "gorilla/ga.h"
#include "gorilla/gau.h"
#include "gorilla/ga_wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ga_StreamContext_File {
  char* filename;
  ga_int32 fileFormat;
  ga_uint32 byteOffset;
  FILE* file;
  /* WAV-Specific Data */
  ga_WavData wavData;
} ga_StreamContext_File;

void gauX_sound_stream_file_produce(ga_HandleStream* in_stream);
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, ga_int32 in_numSamples);
void gauX_sound_stream_file_destroy(ga_HandleStream* in_stream);

ga_Handle* gau_stream_file(ga_Mixer* in_mixer,
                           ga_int32 in_group,
                           ga_int32 in_bufferSize,
                           const char* in_filename,
                           ga_int32 in_fileFormat,
                           ga_uint32 in_byteOffset)
{
  ga_int32 validHdr;
  ga_StreamContext_File* context;
  context = (ga_StreamContext_File*)gaX_cb->allocFunc(sizeof(ga_StreamContext_File));
  context->filename = gaX_cb->allocFunc(strlen(in_filename) + 1);
  strcpy(context->filename, in_filename);
  context->fileFormat = in_fileFormat;
  context->byteOffset = in_byteOffset;
  context->file = fopen(context->filename, "rb");
  if(!context->file)
    goto cleanup;
  if(in_fileFormat == GA_FILE_FORMAT_WAV)
  {
    validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &context->wavData);
    if(validHdr != GA_SUCCESS)
      goto cleanup;
    fseek(context->file, context->wavData.dataOffset, SEEK_SET);
  }
  return ga_handle_createStream(in_mixer,
                                in_group,
                                in_bufferSize,
                                &gauX_sound_stream_file_produce,
                                &gauX_sound_stream_file_seek,
                                &gauX_sound_stream_file_destroy,
                                context);

cleanup:
  if(context->file)
    fclose(context->file);
  gaX_cb->freeFunc(context->filename);
  gaX_cb->freeFunc(context);
  return 0;
}

void gauX_sound_stream_file_produce(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
}
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, ga_int32 in_numSamples)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
}
void gauX_sound_stream_file_destroy(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  fclose(context->file);
  gaX_cb->freeFunc(context->filename);
}
