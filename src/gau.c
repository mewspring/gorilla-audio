#include "gorilla/ga.h"
#include "gorilla/gau.h"
#include "gorilla/ga_wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "vorbis/vorbisfile.h"

/* Ogg Callbacks */
typedef struct gau_OggCallbackData
{
  FILE* rawFile;
  gc_int32 byteOffset;
  gc_int32 rawFileSize;
} gau_OggCallbackData;
size_t gauX_oggRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  gau_OggCallbackData* stream = (gau_OggCallbackData*)datasource;
  FILE* f = stream->rawFile;
  size_t ret = fread(ptr, size, nmemb, f);
  return ret;
}
int gauX_oggSeek(void *datasource, ogg_int64_t offset, int whence)
{
  gau_OggCallbackData* stream = (gau_OggCallbackData*)datasource;
  FILE* f = stream->rawFile;
  switch(whence)
  {
  case SEEK_SET: return fseek(f, (gc_int32)(offset + stream->byteOffset), SEEK_SET);
  case SEEK_CUR: return fseek(f, (gc_int32)offset, SEEK_CUR);
  case SEEK_END: return fseek(f, (gc_int32)((stream->byteOffset + stream->rawFileSize) -  offset), SEEK_SET);
  }
  return -1;
}
long gauX_oggTell(void *datasource)
{
  gau_OggCallbackData* stream = (gau_OggCallbackData*)datasource;
  FILE* f = stream->rawFile;
  return ftell(f) - stream->byteOffset;
}
int gauX_oggClose(void *datasource)
{
  gau_OggCallbackData* stream = (gau_OggCallbackData*)datasource;
  FILE* f = stream->rawFile;
  fclose(f);
  return 1;
}

/* Sound File-Loading Functions */

ga_Sound* gauX_sound_file_from_source(ga_SampleSource* in_sampleSrc)
{
  ga_Sound* ret = 0;
  ga_Format format;
  gc_int32 dataSize;
  gc_int32 totalSamples;
  ga_SampleSource* sampleSrc = in_sampleSrc;
  gc_int32 sampleSize;
  ga_sample_source_format(sampleSrc, &format);
  sampleSize = ga_format_sampleSize(&format);
  ga_sample_source_tell(sampleSrc, &totalSamples);
  if(totalSamples > 0)
  {
    /* Known total samples*/
    char* data;
    dataSize = sampleSize * totalSamples;
    data = gcX_ops->allocFunc(dataSize);
    ga_sample_source_read(sampleSrc, data, totalSamples);
    ret = ga_sound_create(data, dataSize, &format, 0);
    if(!ret)
      gcX_ops->freeFunc(data);
  }
  else
  {
    /* Unknown total samples */
    gc_int32 BUFFER_SAMPLES = format.sampleRate * 2;
    char* data = 0;
    totalSamples = 0;
    while(!ga_sample_source_end(sampleSrc))
    {
      gc_int32 numSamplesRead;
      data = gcX_ops->reallocFunc(data, (totalSamples + BUFFER_SAMPLES) * sampleSize);
      numSamplesRead = ga_sample_source_read(sampleSrc, data + (totalSamples * sampleSize), BUFFER_SAMPLES);
      if(numSamplesRead < BUFFER_SAMPLES)
      {
        data = gcX_ops->reallocFunc(data, (totalSamples + numSamplesRead) * sampleSize);
      }
      totalSamples += numSamplesRead;
    }
    ret = ga_sound_create(data, totalSamples * sampleSize, &format, 0);
    if(!ret)
      gcX_ops->freeFunc(data);
  }
  return ret;
}
ga_Sound* gauX_sound_file_wav(const char* in_filename, gc_uint32 in_byteOffset)
{
  ga_Sound* ret = 0;
  ga_DataSource* dataSrc;
  ga_SampleSource* sampleSrc;
  dataSrc = gau_data_source_create_file_arc(in_filename, in_byteOffset, -1);
  if(dataSrc)
  {
    sampleSrc = gau_sample_source_create_wav(dataSrc);
    if(sampleSrc)
    {
      ret = gauX_sound_file_from_source(sampleSrc);
      ga_sample_source_destroy(sampleSrc);
    }
    ga_data_source_destroy(dataSrc);
  }
  return ret;
}
ga_Sound* gauX_sound_file_ogg(const char* in_filename, gc_uint32 in_byteOffset)
{
  ga_Sound* ret = 0;
  ga_DataSource* dataSrc;
  ga_SampleSource* sampleSrc;
  dataSrc = gau_data_source_create_file_arc(in_filename, in_byteOffset, -1);
  if(dataSrc)
  {
    sampleSrc = gau_sample_source_create_ogg(dataSrc);
    if(sampleSrc)
    {
      ret = gauX_sound_file_from_source(sampleSrc);
      ga_sample_source_destroy(sampleSrc);
    }
    ga_data_source_destroy(dataSrc);
  }
  return ret;
}
ga_Sound* gau_sound_file(const char* in_filename, gc_int32 in_fileFormat,
                        gc_uint32 in_byteOffset)
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
  gc_int32 fileFormat;
  gc_uint32 byteOffset;
  gc_int32 nextSample;
  FILE* file;
  gc_Mutex* seekMutex;
  gc_int32 seek;
  gc_int32 tell;
  gc_Link tellJumps;
  /* WAV-Specific Data */
  union {
    struct {
      ga_WavData wavData;
    } wav;
    struct {
      OggVorbis_File oggFile;
      vorbis_info* oggInfo;
      gau_OggCallbackData oggCallbackData;
      gc_int32 pcmTotal;
    } ogg;
  };
} ga_StreamContext_File;

void gauX_sound_stream_file_produce(ga_HandleStream* in_stream);
void gauX_sound_stream_file_consume(ga_HandleStream* in_handle, gc_int32 in_samplesConsumed);
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, gc_int32 in_sampleOffset);
gc_int32 gauX_sound_stream_file_tell(ga_HandleStream* in_handle, gc_int32 in_param);
void gauX_sound_stream_file_destroy(ga_HandleStream* in_stream);

ga_Handle* gauX_stream_file_wav(ga_Mixer* in_mixer,
                                gc_int32 in_group,
                                gc_int32 in_bufferSize,
                                const char* in_filename,
                                gc_uint32 in_byteOffset,
                                ga_StreamContext_File* in_context)
{
  ga_Format fmt;
  gc_int32 validHdr;
  ga_StreamContext_File* context = in_context;
  validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &context->wav.wavData);
  if(validHdr != GC_SUCCESS)
    return 0;
  fseek(context->file, context->wav.wavData.dataOffset, SEEK_SET);
  fmt.bitsPerSample = context->wav.wavData.bitsPerSample;
  fmt.numChannels = context->wav.wavData.channels;
  fmt.sampleRate = context->wav.wavData.sampleRate;
  return ga_handle_createStream(in_mixer,
                                in_group,
                                in_bufferSize,
                                &fmt,
                                &gauX_sound_stream_file_produce,
                                &gauX_sound_stream_file_consume,
                                &gauX_sound_stream_file_seek,
                                &gauX_sound_stream_file_tell,
                                &gauX_sound_stream_file_destroy,
                                context);
}

ga_Handle* gauX_stream_file_ogg(ga_Mixer* in_mixer,
                                gc_int32 in_group,
                                gc_int32 in_bufferSize,
                                const char* in_filename,
                                gc_uint32 in_byteOffset,
                                ga_StreamContext_File* in_context)
{
  gc_int32 endian = 0; /* 0 is little endian (aka x86), 1 is big endian */
  gc_int32 bytesPerSample = 2;
  gc_int32 totalBytes = 0;
  ga_Format fmt;
  gc_int32 validFile;
  gc_int32 retVal;
  ov_callbacks oggCallbacks;
  oggCallbacks.read_func = &gauX_oggRead;
  oggCallbacks.seek_func = &gauX_oggSeek;
  oggCallbacks.tell_func = &gauX_oggTell;
  oggCallbacks.close_func = &gauX_oggClose;
  fseek(in_context->file, 0, SEEK_END);
  in_context->ogg.oggCallbackData.rawFileSize = ftell(in_context->file);
  fseek(in_context->file, 0, SEEK_SET);
  clearerr(in_context->file);
  in_context->ogg.oggCallbackData.rawFile = in_context->file;
  in_context->ogg.oggCallbackData.byteOffset = in_byteOffset;
  retVal = ov_open_callbacks(&in_context->ogg.oggCallbackData, &in_context->ogg.oggFile, 0, 0, oggCallbacks);
  if(retVal)
    return 0;
  in_context->ogg.oggInfo = ov_info(&in_context->ogg.oggFile, -1);
  ov_pcm_seek(&in_context->ogg.oggFile, 0); /* Seek fixes some poorly-formatted oggs. */
  /* TODO: Verify that ov_pcm_total() == actual number of samples (in debug) */
  in_context->ogg.pcmTotal = (gc_int32)ov_pcm_total(&in_context->ogg.oggFile, -1);
  validFile = in_context->ogg.oggInfo->channels <= 2;
  if(!validFile)
  {
    ov_clear(&in_context->ogg.oggFile);
    in_context->file = 0;
    return 0;
  }
  fmt.bitsPerSample = 16;
  fmt.numChannels = in_context->ogg.oggInfo->channels;
  fmt.sampleRate = in_context->ogg.oggInfo->rate;
  return ga_handle_createStream(in_mixer,
    in_group,
    in_bufferSize,
    &fmt,
    &gauX_sound_stream_file_produce,
    &gauX_sound_stream_file_consume,
    &gauX_sound_stream_file_seek,
    &gauX_sound_stream_file_tell,
    &gauX_sound_stream_file_destroy,
    in_context);
}

ga_Handle* gau_stream_file(ga_Mixer* in_mixer,
                           gc_int32 in_group,
                           gc_int32 in_bufferSize,
                           const char* in_filename,
                           gc_int32 in_fileFormat,
                           gc_uint32 in_byteOffset)
{
  ga_StreamContext_File* context;
  context = (ga_StreamContext_File*)gcX_ops->allocFunc(sizeof(ga_StreamContext_File));
  context->filename = gcX_ops->allocFunc(strlen(in_filename) + 1);
  strcpy(context->filename, in_filename);
  context->fileFormat = in_fileFormat;
  context->byteOffset = in_byteOffset;
  context->file = fopen(context->filename, "rb");
  context->seek = -1;
  context->tell = 0;
  context->nextSample = 0;
  gc_list_head(&context->tellJumps);
  if(context->file)
  {
    context->seekMutex = gc_mutex_create();
    switch(in_fileFormat)
    {
      case GA_FILE_FORMAT_WAV:
        {
          ga_Handle* ret = gauX_stream_file_wav(in_mixer, in_group, in_bufferSize, in_filename, in_byteOffset, context);
          if(ret)
            return ret;
        }
      case GA_FILE_FORMAT_OGG:
        {
          ga_Handle* ret = gauX_stream_file_ogg(in_mixer, in_group, in_bufferSize, in_filename, in_byteOffset, context);
          if(ret)
            return ret;
        }
    }
    gc_mutex_destroy(context->seekMutex);
    fclose(context->file);
  }

  gcX_ops->freeFunc(context->filename);
  gcX_ops->freeFunc(context);
  return 0;
}

gc_int32 gauX_buffer_read_wav_into_buffers(gc_CircBuffer* in_buffer, gc_int32 in_bytes,
                                           FILE* in_file)
{
  void* dataA;
  void* dataB;
  gc_uint32 sizeA;
  gc_uint32 sizeB;
  gc_int32 numBuffers;
  gc_CircBuffer* b = in_buffer;
  gc_int32 bytesToRead = in_bytes;
  numBuffers = gc_buffer_getFree(b, bytesToRead, &dataA, &sizeA, &dataB, &sizeB);
  if(numBuffers >= 1)
  {
    fread(dataA, 1, sizeA, in_file); /* TODO: Is this endian-safe? */
    if(numBuffers == 2)
      fread(dataB, 1, sizeB, in_file); /* TODO: Is this endian-safe? */
  }
  b->nextFree += bytesToRead;
  return bytesToRead;
}

gc_int32 gauX_read_oggvorbis(char* in_data, gc_int32 in_bytes,
                             OggVorbis_File* in_file, vorbis_info* in_info)
{
  gc_int32 samplesLeft = in_bytes / in_info->channels / 2;
  gc_uint32 numBytesRead;
  gc_int32 totalBytes = 0;
  do{
    gc_int32 bitStream;
    gc_float32** samples;
    gc_int32 i;
    gc_int16* dst;
    gc_int32 samplesRead = ov_read_float(in_file, &samples, samplesLeft, &bitStream);
    samplesLeft -= samplesRead;
    numBytesRead = samplesRead * in_info->channels * 2;
    dst = (gc_int16*)(in_data + totalBytes);
    totalBytes += numBytesRead;
    for(i = 0; i < samplesRead; ++i)
    {
      gc_int32 channel;
      for(channel = 0; channel < in_info->channels; ++channel, ++dst)
        *dst = (gc_int16)(samples[channel][i] * 32767.0f);
    }
  } while (numBytesRead > 0 && totalBytes <= in_bytes && samplesLeft);
  return totalBytes;
}

gc_int32 gauX_buffer_read_ogg_into_buffers(gc_CircBuffer* in_buffer, gc_int32 in_bytes,
                                       OggVorbis_File* in_file, vorbis_info* in_info)
{
  void* dataA;
  void* dataB;
  gc_uint32 sizeA = 0;
  gc_uint32 sizeB = 0;
  gc_int32 numBuffers;
  gc_int32 numWritten = 0;
  gc_CircBuffer* b = in_buffer;
  numBuffers = gc_buffer_getFree(b, in_bytes, &dataA, &sizeA, &dataB, &sizeB);
  if(numBuffers >= 1)
  {
    numWritten = gauX_read_oggvorbis(dataA, sizeA, in_file, in_info);
    if(numBuffers == 2 && numWritten == sizeA)
    {
      numWritten += gauX_read_oggvorbis(dataB, sizeB, in_file, in_info);
    }
  }
  b->nextFree += numWritten;
  return numWritten;
}

typedef struct ga_TellJumpData {
  gc_int32 pos;
  gc_int32 delta;
} ga_TellJumpData;

typedef struct ga_TellJumpLink {
  gc_Link link;
  ga_TellJumpData data;
} ga_TellJumpLink;

void gauX_sound_stream_pushTellJump(gc_Link* in_head, gc_int32 in_pos, gc_int32 in_delta)
{
  ga_TellJumpLink* link = gcX_ops->allocFunc(sizeof(ga_TellJumpLink));
  link->link.data = &link->data;
  link->data.pos = in_pos;
  link->data.delta = in_delta;
  gc_list_link(in_head->prev, (gc_Link*)link, &link->data);
}
gc_int32 gauX_sound_stream_peekTellJump(gc_Link* in_head, gc_int32* out_pos, gc_int32* out_delta)
{
  ga_TellJumpLink* link;
  if(in_head->next == in_head)
    return 0;
  link = (ga_TellJumpLink*)in_head->next;
  *out_pos = link->data.pos;
  *out_delta = link->data.delta;
  return 1;
}
gc_int32 gauX_sound_stream_popTellJump(gc_Link* in_head)
{
  ga_TellJumpLink* link;
  if(in_head->next == in_head)
    return 0;
  link = (ga_TellJumpLink*)in_head->next;
  gc_list_unlink((gc_Link*)link);
  gcX_ops->freeFunc(link);
  return 1;
}
gc_int32 gauX_sound_stream_processTellJumps(gc_Link* in_head, gc_int32 in_advance)
{
  gc_int32 ret = 0;
  ga_TellJumpLink* link = (ga_TellJumpLink*)in_head->next;
  while((gc_Link*)link != in_head)
  {
    ga_TellJumpLink* oldLink = link;
    link = (ga_TellJumpLink*)link->link.next;
    oldLink->data.pos -= in_advance;
    if(oldLink->data.pos <= 0)
    {
      ret += oldLink->data.delta;
      gc_list_unlink((gc_Link*)oldLink);
    }
  }
  return ret;
}
void gauX_sound_stream_clearTellJumps(gc_Link* in_head)
{
  while(gauX_sound_stream_popTellJump(in_head));
}

/* Complex */
void gauX_sound_stream_file_produce(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  ga_HandleStream* h = in_handle;
  gc_CircBuffer* b = h->buffer;
  gc_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
  gc_int32 bytesFree = gc_buffer_bytesFree(b);
  gc_int32 loop;
  gc_int32 loopStart;
  gc_int32 loopEnd;
  gc_mutex_lock(h->handleMutex);
  loop = h->loop;
  loopStart = h->loopStart;
  loopEnd = h->loopEnd;
  gc_mutex_unlock(h->handleMutex);

  if(context->seek >= 0)
  {
    gc_int32 samplePos;
    gc_mutex_lock(h->consumeMutex);
    gc_mutex_lock(context->seekMutex);
    if(context->seek >= 0) /* Check again now that we're mutexed */
    {
      samplePos = context->seek;
      context->tell = samplePos;
      context->seek = -1;
      context->nextSample = samplePos;
      switch(context->fileFormat)
      {
      case GA_FILE_FORMAT_WAV:
        {
          gc_int32 totalSamples = context->wav.wavData.dataSize / sampleSize;
          samplePos = samplePos > totalSamples ? 0 : samplePos;
          fseek(context->file, context->wav.wavData.dataOffset + samplePos * sampleSize, SEEK_SET);
          break;
        }
      case GA_FILE_FORMAT_OGG:
        {
          /* TODO: Make sure we're not seeking to an invalid location */
          ov_pcm_seek(&context->ogg.oggFile, samplePos);
          break;
        }
      }
      gc_buffer_consume(h->buffer, gc_buffer_bytesAvail(h->buffer)); /* Clear buffer */
      gauX_sound_stream_clearTellJumps(&context->tellJumps); /* Clear tell-jump list */
    }
    gc_mutex_unlock(context->seekMutex);
    gc_mutex_unlock(h->consumeMutex);
  }

  while(bytesFree)
  {
    gc_int32 bytesWritten = 0;
    gc_int32 bytesToWrite =
      (loop && loopEnd > context->nextSample) ?
      (loopEnd - context->nextSample) * sampleSize:
      bytesFree;
    bytesToWrite = bytesToWrite > bytesFree ? bytesFree : bytesToWrite;
    switch(context->fileFormat)
    {
    case GA_FILE_FORMAT_WAV:
      {
        gc_int32 wavAvail = context->wav.wavData.dataSize - context->nextSample * sampleSize;
        gc_int32 bytesToRead = bytesToWrite > wavAvail ? wavAvail : bytesToWrite;
        bytesWritten = gauX_buffer_read_wav_into_buffers(b, bytesToRead, context->file);
        break;
      }
    case GA_FILE_FORMAT_OGG:
      {
        bytesWritten = gauX_buffer_read_ogg_into_buffers(b, bytesToWrite, &context->ogg.oggFile, context->ogg.oggInfo);
        break;
      }
    }
    bytesFree -= bytesWritten;
    context->nextSample += bytesWritten / sampleSize;
    if(bytesWritten < bytesToWrite || (loop && context->nextSample == loopEnd))
    {
      if(loop)
      {
        switch(context->fileFormat)
        {
        case GA_FILE_FORMAT_WAV:
          {
            fseek(context->file, context->wav.wavData.dataOffset + loopStart * sampleSize, SEEK_SET);
            break;
          }
        case GA_FILE_FORMAT_OGG:
          {
            ov_pcm_seek(&context->ogg.oggFile, 0);
            break;
          }
        }
        gc_mutex_unlock(h->consumeMutex);
        gc_mutex_lock(context->seekMutex);
        /* Add tell jump */
        gauX_sound_stream_pushTellJump(
          &context->tellJumps,
          gc_buffer_bytesAvail(h->buffer) / sampleSize,
          loopStart - context->nextSample);
        gc_mutex_unlock(context->seekMutex);
        gc_mutex_unlock(h->consumeMutex);
        context->nextSample = loopStart;
      }
      else
      {
        h->produceFunc = 0;
        break;
      }
    }
  }
}

/* Simple */
void gauX_sound_stream_file_consume(ga_HandleStream* in_handle, gc_int32 in_samplesConsumed)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  gc_int32 delta;
  gc_mutex_lock(context->seekMutex);
  context->tell += in_samplesConsumed;
  delta = gauX_sound_stream_processTellJumps(&context->tellJumps, in_samplesConsumed);
  context->tell += delta;
  gc_mutex_unlock(context->seekMutex);
}
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, gc_int32 in_sampleOffset)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  gc_mutex_lock(context->seekMutex);
  context->seek = in_sampleOffset;
  gc_mutex_unlock(context->seekMutex);
}
gc_int32 gauX_sound_stream_file_tell(ga_HandleStream* in_handle, gc_int32 in_param)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  gc_int32 ret = -1;
  if(in_param == GA_TELL_PARAM_TOTAL)
  {
    switch(context->fileFormat)
    {
    case GA_FILE_FORMAT_WAV:
      {
        gc_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
        ret = context->wav.wavData.dataSize / sampleSize;
        break;
      }
    case GA_FILE_FORMAT_OGG:
      {
        gc_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
        gc_int32 totalSamples = context->ogg.pcmTotal;
        ret = totalSamples;
        break;
      }
    }
  }
  else
  {
    gc_mutex_lock(context->seekMutex);
    ret = context->seek >= 0 ? context->seek : context->tell;
    gc_mutex_unlock(context->seekMutex);
  }
  return ret;
}
void gauX_sound_stream_file_destroy(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  gc_mutex_destroy(context->seekMutex);
  gauX_sound_stream_clearTellJumps(&context->tellJumps);
  gcX_ops->freeFunc(context->filename);
  switch(context->fileFormat)
  {
  case GA_FILE_FORMAT_WAV:
    {
      fclose(context->file);
      break;
    }
  case GA_FILE_FORMAT_OGG:
    {
      ov_clear(&context->ogg.oggFile);
      break;
    }
  }
}

gc_result gau_sound_file_format(const char* in_filename,
                                gc_int32 in_fileFormat,
                                gc_uint32 in_byteOffset,
                                ga_Format* out_format)
{
  gc_result ret = GC_ERROR_GENERIC;
  switch(in_fileFormat)
  {
  case GA_FILE_FORMAT_WAV:
    {
      ga_WavData wavData;
      ret = gaX_sound_load_wav_header(in_filename, in_byteOffset, &wavData);
      if(ret == GC_SUCCESS)
      {
        out_format->bitsPerSample = wavData.bitsPerSample;
        out_format->numChannels = wavData.channels;
        out_format->sampleRate = wavData.sampleRate;
      }
      break;
    }
  case GA_FILE_FORMAT_OGG:
    {
      vorbis_info* oggInfo;
      OggVorbis_File oggFile;
      gc_int32 validFile;
      gc_int32 retVal = 0;
      gau_OggCallbackData oggCallbackData;
      ov_callbacks oggCallbacks;
      FILE* rawFile = fopen(in_filename, "rb");
      if(!rawFile)
        break;
      oggCallbacks.read_func = &gauX_oggRead;
      oggCallbacks.seek_func = &gauX_oggSeek;
      oggCallbacks.tell_func = &gauX_oggTell;
      oggCallbacks.close_func = &gauX_oggClose;
      fseek(rawFile, 0, SEEK_END);
      oggCallbackData.rawFileSize = ftell(rawFile);
      fseek(rawFile, 0, SEEK_SET);
      clearerr(rawFile);
      oggCallbackData.rawFile = rawFile;
      oggCallbackData.byteOffset = in_byteOffset;
      retVal = ov_open_callbacks(&oggCallbackData, &oggFile, 0, 0, oggCallbacks);
      if(!retVal)
      {
        oggInfo = ov_info(&oggFile, -1);
        ov_pcm_seek(&oggFile, 0); /* Seek fixes some poorly-formatted oggs. */
        validFile = oggInfo->channels <= 2;
        if(validFile)
        {
          out_format->bitsPerSample = 2 * 8; /* always read oggs as 16-bit */
          out_format->numChannels = oggInfo->channels;
          out_format->sampleRate = oggInfo->rate;
          ret = GC_SUCCESS;
        }
        ov_clear(&oggFile);
      }
      else
        fclose(rawFile);
      break;
    }
  }
  return ret;
}

/* File-Based Data Source */
typedef struct gau_DataSourceFileContext {
  FILE* f;
} gau_DataSourceFileContext;

typedef struct gau_DataSourceFile {
  ga_DataSource dataSrc;
  gau_DataSourceFileContext context;
} gau_DataSourceFile;

gc_int32 gauX_data_source_file_read(void* in_context, void* in_dst, gc_int32 in_size, gc_int32 in_count)
{
  gau_DataSourceFileContext* ctx = (gau_DataSourceFileContext*)in_context;
  return (gc_int32)fread(in_dst, in_size, in_count, ctx->f);
}
gc_int32 gauX_data_source_file_seek(void* in_context, gc_int32 in_offset, gc_int32 in_origin)
{
  gau_DataSourceFileContext* ctx = (gau_DataSourceFileContext*)in_context;
  switch(in_origin)
  {
  case GA_SEEK_ORIGIN_SET: fseek(ctx->f, in_offset, SEEK_SET); break;
  case GA_SEEK_ORIGIN_CUR: fseek(ctx->f, in_offset, SEEK_CUR); break;
  case GA_SEEK_ORIGIN_END: fseek(ctx->f, in_offset, SEEK_END); break;
  }
  return 0;
}
gc_int32 gauX_data_source_file_tell(void* in_context)
{
  gau_DataSourceFileContext* ctx = (gau_DataSourceFileContext*)in_context;
  return ftell(ctx->f);
}
void gauX_data_source_file_close(void* in_context)
{
  gau_DataSourceFileContext* ctx = (gau_DataSourceFileContext*)in_context;
  fclose(ctx->f);
}
ga_DataSource* gau_data_source_create_file(const char* in_filename)
{
  gau_DataSourceFile* ret = gcX_ops->allocFunc(sizeof(gau_DataSourceFile));
  ret->dataSrc.readFunc = &gauX_data_source_file_read;
  ret->dataSrc.seekFunc = &gauX_data_source_file_seek;
  ret->dataSrc.tellFunc = &gauX_data_source_file_tell;
  ret->dataSrc.closeFunc = &gauX_data_source_file_close;
  ret->context.f = fopen(in_filename, "rb");
  if(!ret->context.f)
  {
    gcX_ops->freeFunc(ret);
    ret = 0;
  }
  return (ga_DataSource*)ret;
}

/* File-Based Archived Data Source */
typedef struct gau_DataSourceFileArcContext {
  gc_int32 offset;
  gc_int32 size;
  FILE* f;
} gau_DataSourceFileArcContext;

typedef struct gau_DataSourceFileArc {
  ga_DataSource dataSrc;
  gau_DataSourceFileArcContext context;
} gau_DataSourceFileArc;

gc_int32 gauX_data_source_file_arc_read(void* in_context, void* in_dst, gc_int32 in_size, gc_int32 in_count)
{
  gau_DataSourceFileArcContext* ctx = (gau_DataSourceFileArcContext*)in_context;
  /* TODO: Implement in-archive EOF detection? */
  return (gc_int32)fread(in_dst, in_size, in_count, ctx->f);
}
gc_int32 gauX_data_source_file_arc_seek(void* in_context, gc_int32 in_offset, gc_int32 in_origin)
{
  /* TODO: What is the best way to resolve the seeking-OOB cases? */
  gau_DataSourceFileArcContext* ctx = (gau_DataSourceFileArcContext*)in_context;
  switch(in_origin)
  {
  case GA_SEEK_ORIGIN_SET:
    if(ctx->size > 0 && in_offset > ctx->size)
      return -1;
    fseek(ctx->f, ctx->offset + in_offset, SEEK_SET);
    break;
  case GA_SEEK_ORIGIN_CUR:
    {
      gc_int32 curPos = ftell(ctx->f) - ctx->offset;
      gc_int32 newPos = curPos + in_offset;
      if(newPos < 0 || (ctx->size > 0 && newPos > ctx->size))
        return -1;
      fseek(ctx->f, in_offset, SEEK_CUR);
    }
    break;
  case GA_SEEK_ORIGIN_END:
    if(ctx->size == 0)
      return -1;
    fseek(ctx->f, ctx->offset + ctx->size + in_offset, SEEK_SET);
    break;
  }
  return 0;
}
gc_int32 gauX_data_source_file_arc_tell(void* in_context)
{
  gau_DataSourceFileArcContext* ctx = (gau_DataSourceFileArcContext*)in_context;
  return ftell(ctx->f) - ctx->offset;
}
void gauX_data_source_file_arc_close(void* in_context)
{
  gau_DataSourceFileArcContext* ctx = (gau_DataSourceFileArcContext*)in_context;
  fclose(ctx->f);
}
ga_DataSource* gau_data_source_create_file_arc(const char* in_filename, gc_int32 in_offset, gc_int32 in_size)
{
  gau_DataSourceFileArc* ret = gcX_ops->allocFunc(sizeof(gau_DataSourceFileArc));
  ret->dataSrc.readFunc = &gauX_data_source_file_arc_read;
  ret->dataSrc.seekFunc = &gauX_data_source_file_arc_seek;
  ret->dataSrc.tellFunc = &gauX_data_source_file_arc_tell;
  ret->dataSrc.closeFunc = &gauX_data_source_file_arc_close;
  ret->context.offset = in_offset;
  ret->context.size = in_size;
  ret->context.f = fopen(in_filename, "rb");
  if(!ret->context.f)
  {
    gcX_ops->freeFunc(ret);
    ret = 0;
  }
  fseek(ret->context.f, in_offset, SEEK_SET);
  return (ga_DataSource*)ret;
}

/* WAV Sample Source */
gc_result gauX_sample_source_wav_load_header(ga_DataSource* in_dataSrc, ga_WavData* out_wavData)
{
  /* TODO: Make this work with non-blocking reads? Need to get this data... */
  char id[5];
  ga_WavData* wavData = out_wavData;
  if(!in_dataSrc)
    return GC_ERROR_GENERIC;
  ga_data_source_read(in_dataSrc, &id[0], sizeof(char), 4); /* 'RIFF' */
  id[4] = 0;
  if(!strcmp(id, "RIFF"))
  {
    ga_data_source_read(in_dataSrc, &wavData->fileSize, sizeof(gc_int32), 1);
    ga_data_source_read(in_dataSrc, &id[0], sizeof(char), 4); /* 'WAVE' */
    if(!strcmp(id, "WAVE"))
    {
      gc_int32 seekSuccess;
      gc_int32 dataFound;
      gc_int32 fmtStart;
      ga_data_source_read(in_dataSrc, &id[0], sizeof(char), 4); /* 'fmt ' */
      ga_data_source_read(in_dataSrc, &wavData->fmtSize, sizeof(gc_int32), 1);
      fmtStart = ga_data_source_tell(in_dataSrc);
      assert(fmtStart >= 0); /* TODO: Support data sources without seek/tell support */
      ga_data_source_read(in_dataSrc, &wavData->fmtTag, sizeof(gc_int16), 1);
      ga_data_source_read(in_dataSrc, &wavData->channels, sizeof(gc_int16), 1);
      ga_data_source_read(in_dataSrc, &wavData->sampleRate, sizeof(gc_int32), 1);
      ga_data_source_read(in_dataSrc, &wavData->bytesPerSec, sizeof(gc_int32), 1);
      ga_data_source_read(in_dataSrc, &wavData->blockAlign, sizeof(gc_int16), 1);
      ga_data_source_read(in_dataSrc, &wavData->bitsPerSample, sizeof(gc_int16), 1);
      seekSuccess = ga_data_source_seek(in_dataSrc, fmtStart + wavData->fmtSize, GA_SEEK_ORIGIN_SET);
      assert(seekSuccess >= 0); /* TODO: Support data sources without seek/tell support */
      do
      {
        ga_data_source_read(in_dataSrc, &id[0], sizeof(char), 4); /* 'data' */
        ga_data_source_read(in_dataSrc, &wavData->dataSize, sizeof(gc_int32), 1);
        dataFound = !strcmp(id, "data");
        if(dataFound)
        {
          wavData->dataOffset = ga_data_source_tell(in_dataSrc);
          assert(wavData->dataOffset >= 0); /* TODO: Support data sources without seek/tell support */
        }
        else
        {
          seekSuccess = ga_data_source_seek(in_dataSrc, wavData->dataSize, GA_SEEK_ORIGIN_CUR);
          assert(seekSuccess >= 0); /* TODO: Support data sources without seek/tell support */
        }
      } while(!dataFound/* && !feof(f) */); /* TODO: Need End-Of-Data support in Data Sources */
      if(dataFound)
        return GC_SUCCESS;
    }
  }
  return GC_ERROR_GENERIC;
}

typedef struct gau_SampleSourceWavContext {
  ga_DataSource* dataSrc;
  ga_WavData wavHeader;
  gc_int32 sampleSize;
  gc_int32 pos;
} gau_SampleSourceWavContext;

typedef struct gau_SampleSourceWav {
  ga_SampleSource sampleSrc;
  gau_SampleSourceWavContext context;
} gau_SampleSourceWav;

gc_int32 gauX_sample_source_wav_read(void* in_context, void* in_dst, gc_int32 in_numSamples)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
  gc_int32 numRead = 0;
  gc_int32 totalSamples = ctx->wavHeader.dataSize / ctx->sampleSize;
  if(ctx->pos + in_numSamples > totalSamples)
    in_numSamples = totalSamples - ctx->pos;
  if(in_numSamples > 0)
  {
    numRead = ga_data_source_read(ctx->dataSrc, in_dst, ctx->sampleSize, in_numSamples);
    ctx->pos += numRead;
  }
  return numRead;
}
gc_int32 gauX_sample_source_wav_end(void* in_context)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
  gc_int32 totalSamples = ctx->wavHeader.dataSize / ctx->sampleSize;
  return ctx->pos == totalSamples;
}
gc_int32 gauX_sample_source_wav_seek(void* in_context, gc_int32 in_sampleOffset)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
  gc_int32 ret = ga_data_source_seek(ctx->dataSrc, ctx->wavHeader.dataOffset + in_sampleOffset * ctx->sampleSize, GA_SEEK_ORIGIN_SET);
  if(ret >= 0)
    ctx->pos = in_sampleOffset;
  return ret;
}
gc_int32 gauX_sample_source_wav_tell(void* in_context, gc_int32* out_totalSamples)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
  if(out_totalSamples)
    *out_totalSamples = ctx->wavHeader.dataSize / ctx->sampleSize;
  return ctx->pos;
}
void gauX_sample_source_wav_close(void* in_context)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
}
ga_SampleSource* gau_sample_source_create_wav(ga_DataSource* in_dataSrc)
{
  gc_result validHeader;
  gau_SampleSourceWav* ret = gcX_ops->allocFunc(sizeof(gau_SampleSourceWav));
  gau_SampleSourceWavContext* ctx = &ret->context;
  ret->sampleSrc.readFunc = &gauX_sample_source_wav_read;
  ret->sampleSrc.endFunc = &gauX_sample_source_wav_end;
  ret->sampleSrc.seekFunc = &gauX_sample_source_wav_seek;
  ret->sampleSrc.tellFunc = &gauX_sample_source_wav_tell;
  ret->sampleSrc.closeFunc = &gauX_sample_source_wav_close;
  ctx->pos = 0;
  ctx->dataSrc = in_dataSrc;
  validHeader = gauX_sample_source_wav_load_header(in_dataSrc, &ctx->wavHeader);
  if(validHeader == GC_SUCCESS)
  {
    ret->sampleSrc.format.numChannels = ctx->wavHeader.channels;
    ret->sampleSrc.format.bitsPerSample = ctx->wavHeader.bitsPerSample;
    ret->sampleSrc.format.sampleRate = ctx->wavHeader.sampleRate;
    ctx->sampleSize = ga_format_sampleSize(&ret->sampleSrc.format);
  }
  else
  {
    gcX_ops->freeFunc(ret);
    ret = 0;
  }
  return (ga_SampleSource*)ret;
}

/* OGG Sample Source */
typedef struct gau_OggDataSourceCallbackData
{
  ga_DataSource* dataSrc;
} gau_OggDataSourceCallbackData;

size_t gauX_sample_source_ogg_callback_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  gau_OggDataSourceCallbackData* stream = (gau_OggDataSourceCallbackData*)datasource;
  ga_DataSource* ds = stream->dataSrc;
  size_t ret = (size_t)ga_data_source_read(ds, ptr, size, nmemb);
  return ret;
}
int gauX_sample_source_ogg_callback_seek(void *datasource, ogg_int64_t offset, int whence)
{
  gau_OggDataSourceCallbackData* stream = (gau_OggDataSourceCallbackData*)datasource;
  ga_DataSource* ds = stream->dataSrc;
  switch(whence)
  {
  case SEEK_SET: return ga_data_source_seek(ds, (gc_int32)offset, GA_SEEK_ORIGIN_SET);
  case SEEK_CUR: return ga_data_source_seek(ds, (gc_int32)offset, GA_SEEK_ORIGIN_CUR);
  case SEEK_END: return ga_data_source_seek(ds, (gc_int32)offset, GA_SEEK_ORIGIN_END);
  }
  return -1;
}
long gauX_sample_source_ogg_callback_tell(void *datasource)
{
  gau_OggDataSourceCallbackData* stream = (gau_OggDataSourceCallbackData*)datasource;
  ga_DataSource* ds = stream->dataSrc;
  return ga_data_source_tell(ds);
}
int gauX_sample_source_ogg_callback_close(void *datasource)
{
  gau_OggDataSourceCallbackData* stream = (gau_OggDataSourceCallbackData*)datasource;
  ga_DataSource* ds = stream->dataSrc;
  return 1;
}

typedef struct gau_SampleSourceOggContext {
  ga_DataSource* dataSrc;
  gc_int32 endOfSamples;
  OggVorbis_File oggFile;
  vorbis_info* oggInfo;
  gau_OggDataSourceCallbackData oggCallbackData;
} gau_SampleSourceOggContext;

typedef struct gau_SampleSourceOgg {
  ga_SampleSource sampleSrc;
  gau_SampleSourceOggContext context;
} gau_SampleSourceOgg;

gc_int32 gauX_sample_source_ogg_read(void* in_context, void* in_dst, gc_int32 in_numSamples)
{
  gau_SampleSourceOggContext* ctx = (gau_SampleSourceOggContext*)in_context;
  gc_int32 samplesLeft = in_numSamples;
  gc_int32 samplesRead;
  gc_int32 totalSamples = 0;
  do{
    gc_int32 bitStream;
    gc_float32** samples;
    gc_int32 i;
    gc_int16* dst;
    gc_int32 channel;
    samplesRead = ov_read_float(&ctx->oggFile, &samples, samplesLeft, &bitStream);
    if(samplesRead == 0)
      ctx->endOfSamples = 1;
    else if(samplesRead > 0)
    {
      samplesLeft -= samplesRead;
      dst = (gc_int16*)(in_dst) + totalSamples * ctx->oggInfo->channels;
      totalSamples += samplesRead;
      for(i = 0; i < samplesRead; ++i)
      {
        for(channel = 0; channel < ctx->oggInfo->channels; ++channel, ++dst)
          *dst = (gc_int16)(samples[channel][i] * 32767.0f);
      }
    }
  } while (samplesRead > 0 && samplesLeft);
  return totalSamples;
}
gc_int32 gauX_sample_source_ogg_end(void* in_context)
{
  gau_SampleSourceOggContext* ctx = (gau_SampleSourceOggContext*)in_context;
  return ctx->endOfSamples;
}
gc_int32 gauX_sample_source_ogg_seek(void* in_context, gc_int32 in_sampleOffset)
{
  gau_SampleSourceOggContext* ctx = (gau_SampleSourceOggContext*)in_context;
  gc_int32 ret = ov_pcm_seek(&ctx->oggFile, in_sampleOffset);
  ctx->endOfSamples = 0;
  return ret;
}
gc_int32 gauX_sample_source_ogg_tell(void* in_context, gc_int32* out_totalSamples)
{
  gau_SampleSourceOggContext* ctx = (gau_SampleSourceOggContext*)in_context;
  /* TODO: Decide whether to support total samples for OGG files */
  if(out_totalSamples)
    *out_totalSamples = 0/*ov_pcm_total()*/;
  return (gc_int32)ov_pcm_tell(&ctx->oggFile);
}
void gauX_sample_source_ogg_close(void* in_context)
{
  gau_SampleSourceWavContext* ctx = (gau_SampleSourceWavContext*)in_context;
}
ga_SampleSource* gau_sample_source_create_ogg(ga_DataSource* in_dataSrc)
{
  gau_SampleSourceOgg* ret = gcX_ops->allocFunc(sizeof(gau_SampleSourceOgg));
  gau_SampleSourceOggContext* ctx = &ret->context;
  gc_int32 endian = 0; /* 0 is little endian (aka x86), 1 is big endian */
  gc_int32 bytesPerSample = 2;
  gc_int32 isValidOgg = 0;
  gc_int32 oggIsOpen;
  ov_callbacks oggCallbacks;
  ret->sampleSrc.readFunc = &gauX_sample_source_ogg_read;
  ret->sampleSrc.endFunc = &gauX_sample_source_ogg_end;
  ret->sampleSrc.seekFunc = &gauX_sample_source_ogg_seek;
  ret->sampleSrc.tellFunc = &gauX_sample_source_ogg_tell;
  ret->sampleSrc.closeFunc = &gauX_sample_source_ogg_close;
  ctx->dataSrc = in_dataSrc;
  ctx->endOfSamples = 0;

  /* OGG Setup */
  oggCallbacks.read_func = &gauX_sample_source_ogg_callback_read;
  oggCallbacks.seek_func = &gauX_sample_source_ogg_callback_seek;
  oggCallbacks.tell_func = &gauX_sample_source_ogg_callback_tell;
  oggCallbacks.close_func = &gauX_sample_source_ogg_callback_close;
  ctx->oggCallbackData.dataSrc = in_dataSrc;
  oggIsOpen = ov_open_callbacks(&ctx->oggCallbackData, &ctx->oggFile, 0, 0, oggCallbacks);
  if(oggIsOpen == 0) /* 0 means "open" */
  {
    ctx->oggInfo = ov_info(&ctx->oggFile, -1);
    ov_pcm_seek(&ctx->oggFile, 0); /* Seek fixes some poorly-formatted OGGs. */
    isValidOgg = ctx->oggInfo->channels <= 2;
    if(isValidOgg)
    {
      ret->sampleSrc.format.bitsPerSample = bytesPerSample * 8;
      ret->sampleSrc.format.numChannels = ctx->oggInfo->channels;
      ret->sampleSrc.format.sampleRate = ctx->oggInfo->rate;
    }
    else
      ov_clear(&ctx->oggFile);
  }
  if(!isValidOgg)
  {
    gcX_ops->freeFunc(ret);
    ret = 0;
  }
  return (ga_SampleSource*)ret;
}
