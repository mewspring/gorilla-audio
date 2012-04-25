#include "gorilla/ga.h"
#include "gorilla/gau.h"
#include "gorilla/ga_wav.h"
#include "gorilla/ga_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sound File-Loading Functions */
ga_Sound* gauX_sound_file_wav(const char* in_filename, ga_uint32 in_byteOffset)
{
  ga_Sound* ret;
  FILE* f;
  ga_WavData wavData;
  char* data;
  ga_Format format;
  ga_int32 validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &wavData);
  if(validHdr != GA_SUCCESS)
    return 0;
  f = fopen(in_filename, "rb");
  if(!f)
    return 0;
  data = gaX_cb->allocFunc(wavData.dataSize);
  fseek(f, wavData.dataOffset, SEEK_SET);
  fread(data, 1, wavData.dataSize, f);
  fclose(f);
  format.bitsPerSample = wavData.bitsPerSample;
  format.numChannels = wavData.channels;
  format.sampleRate = wavData.sampleRate;
  ret = ga_sound_create(data, wavData.dataSize, &format, 0);
  if(!ret)
    gaX_cb->freeFunc(data);
  return ret;
}
ga_Sound* gauX_sound_file_ogg(const char* in_filename, ga_uint32 in_byteOffset)
{
  /* TODO! */
  return 0;
}
ga_Sound* gau_sound_file(const char* in_filename, ga_int32 in_fileFormat,
                        ga_uint32 in_byteOffset)
{
  switch(in_fileFormat)
  {
  case GA_FILE_FORMAT_WAV:
    return (ga_Sound*)gauX_sound_file_wav(in_filename, in_byteOffset);

  case GA_FILE_FORMAT_OGG:
    return (ga_Sound*)gauX_sound_file_ogg(in_filename, in_byteOffset);
  }
  return 0;
}

/* Streaming Functions */
typedef struct ga_StreamContext_File {
  char* filename;
  ga_int32 fileFormat;
  ga_uint32 byteOffset;
  ga_int32 nextSample;
  FILE* file;
  ga_Mutex* seekMutex;
  ga_int32 seek;
  ga_int32 tell;
  /* WAV-Specific Data */
  union {
    struct {
      ga_WavData wavData;
    } wav;
  };
} ga_StreamContext_File;

void gauX_sound_stream_file_produce(ga_HandleStream* in_stream);
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, ga_int32 in_sampleOffset);
ga_int32 gauX_sound_stream_file_tell(ga_HandleStream* in_handle, ga_int32 in_param);
void gauX_sound_stream_file_destroy(ga_HandleStream* in_stream);

ga_Handle* gau_stream_file(ga_Mixer* in_mixer,
                           ga_int32 in_group,
                           ga_int32 in_bufferSize,
                           const char* in_filename,
                           ga_int32 in_fileFormat,
                           ga_uint32 in_byteOffset)
{
  ga_int32 validHdr;
  ga_Format fmt;
  ga_StreamContext_File* context;
  context = (ga_StreamContext_File*)gaX_cb->allocFunc(sizeof(ga_StreamContext_File));
  context->filename = gaX_cb->allocFunc(strlen(in_filename) + 1);
  strcpy(context->filename, in_filename);
  context->fileFormat = in_fileFormat;
  context->byteOffset = in_byteOffset;
  context->file = fopen(context->filename, "rb");
  context->seek = -1;
  context->tell = 0;
  context->nextSample = 0;
  if(!context->file)
    goto cleanup;
  if(in_fileFormat == GA_FILE_FORMAT_WAV)
  {
    validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &context->wav.wavData);
    if(validHdr != GA_SUCCESS)
      goto cleanup;
    fseek(context->file, context->wav.wavData.dataOffset, SEEK_SET);
    fmt.bitsPerSample = context->wav.wavData.bitsPerSample;
    fmt.numChannels = context->wav.wavData.channels;
    fmt.sampleRate = context->wav.wavData.sampleRate;
    context->seekMutex = ga_mutex_create();
    return ga_handle_createStream(in_mixer,
                                  in_group,
                                  in_bufferSize,
                                  &fmt,
                                  &gauX_sound_stream_file_produce,
                                  &gauX_sound_stream_file_seek,
                                  &gauX_sound_stream_file_tell,
                                  &gauX_sound_stream_file_destroy,
                                  context);
  }

cleanup:
  if(context->file)
    fclose(context->file);
  gaX_cb->freeFunc(context->filename);
  gaX_cb->freeFunc(context);
  return 0;
}

void gauX_buffer_read_into_buffers(ga_CircBuffer* in_buffer, ga_int32 in_bytes,
                                   FILE* in_file)
{
  void* dataA;
  void* dataB;
  ga_uint32 sizeA;
  ga_uint32 sizeB;
  ga_int32 numBuffers;
  ga_CircBuffer* b = in_buffer;
  numBuffers = ga_buffer_getFree(b, in_bytes, &dataA, &sizeA, &dataB, &sizeB);
  if(numBuffers >= 1)
  {
    fread(dataA, 1, sizeA, in_file); /* TODO: Is this endian-safe? */
    if(numBuffers == 2)
      fread(dataB, 1, sizeB, in_file); /* TODO: Is this endian-safe? */
  }
  b->nextFree += in_bytes;
}
void gauX_sound_stream_file_produce(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  ga_HandleStream* h = in_handle;
  ga_CircBuffer* b = h->buffer;
  ga_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
  ga_int32 totalSamples = context->wav.wavData.dataSize / sampleSize;
  ga_int32 bytesFree = ga_buffer_bytesFree(b);

  ga_int32 loop;
  ga_int32 loopStart;
  ga_int32 loopEnd;
  ga_mutex_lock(h->handleMutex);
  loop = h->loop;
  loopStart = h->loopStart;
  loopEnd = h->loopEnd;
  ga_mutex_unlock(h->handleMutex);
  loopEnd = loopEnd < 0 ? totalSamples : loopEnd;

  if(context->seek >= 0)
  {
    ga_int32 samplePos;
    ga_mutex_lock(h->consumeMutex);
    ga_mutex_lock(context->seekMutex);
    samplePos = context->seek;
    samplePos = samplePos > totalSamples ? 0 : samplePos;
    context->tell = samplePos;
    context->seek = -1;
    context->nextSample = samplePos;
    fseek(context->file, context->wav.wavData.dataOffset + samplePos * sampleSize, SEEK_SET);
    ga_buffer_consume(h->buffer, ga_buffer_bytesAvail(h->buffer)); /* Clear buffer */
    ga_mutex_unlock(context->seekMutex);
    ga_mutex_unlock(h->consumeMutex);

    /* TODO: Clear tell-jump list */
  }

  while(bytesFree)
  {
    ga_int32 endSample = (loop && context->nextSample < loopEnd) ? loopEnd : totalSamples;
    ga_int32 bytesToWrite = (endSample - context->nextSample) * sampleSize;
    bytesToWrite = bytesToWrite > bytesFree ? bytesFree : bytesToWrite;
    gauX_buffer_read_into_buffers(b, bytesToWrite, context->file);
    bytesFree -= bytesToWrite;
    context->nextSample += bytesToWrite / sampleSize;
    if(context->nextSample >= endSample)
    {
      if(loop)
      {
        fseek(context->file, context->wav.wavData.dataOffset + loopStart * sampleSize, SEEK_SET);
        context->nextSample = loopStart;
        /* TODO: Add tell jump */
      }
      else
      {
        h->produceFunc = 0;
        break;
      }
    }
  }
}
void gauX_sound_stream_file_consume(ga_HandleStream* in_handle, ga_int32 in_samples)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  context->tell += in_samples;
  /* TODO: Check tell jumps */
}
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, ga_int32 in_sampleOffset)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  ga_mutex_lock(context->seekMutex);
  context->seek = in_sampleOffset;
  ga_mutex_unlock(context->seekMutex);
}
ga_int32 gauX_sound_stream_file_tell(ga_HandleStream* in_handle, ga_int32 in_param)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  ga_int32 ret;
  ga_mutex_lock(context->seekMutex);
  ret = context->seek >= 0 ? context->seek : context->tell;
  ga_mutex_unlock(context->seekMutex);
  return ret;
}
void gauX_sound_stream_file_destroy(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  ga_mutex_destroy(context->seekMutex);
  fclose(context->file);
  gaX_cb->freeFunc(context->filename);
}
