#include "gorilla/ga.h"
#include "gorilla/gau.h"
#include "gorilla/ga_wav.h"
#include "gorilla/ga_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vorbis/vorbisfile.h"

/* Ogg Callbacks */
typedef struct gau_OggCallbackData
{
  FILE* rawFile;
  ga_int32 byteOffset;
  ga_int32 rawFileSize;
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
  case SEEK_SET: return fseek(f, (ga_int32)(offset + stream->byteOffset), SEEK_SET);
  case SEEK_CUR: return fseek(f, (ga_int32)offset, SEEK_CUR);
  case SEEK_END: return fseek(f, (ga_int32)((stream->byteOffset + stream->rawFileSize) -  offset), SEEK_SET);
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
  ga_Sound* ret = 0;
  ga_int32 endian = 0; // 0 is little endian (aka x86), 1 is big endian
  ga_int32 bytesPerSample = 2;
  ga_int32 totalBytes = 0;
  char* data = 0;
  ga_Format format;
  vorbis_info* oggInfo;
  OggVorbis_File oggFile;
  ga_int32 validFile;
  ga_int32 retVal = 0;
  gau_OggCallbackData oggCallbackData;
  FILE* rawFile = fopen(in_filename, "rb");
  ov_callbacks oggCallbacks;
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
  if(retVal)
    return 0;
  oggInfo = ov_info(&oggFile, -1);
  ov_pcm_seek(&oggFile, 0); /* Seek fixes some poorly-formatted oggs. */
  validFile = oggInfo->channels <= 2;
  if(validFile)
  {
    ga_int32 bufferSize = 64 * 1024 / oggInfo->channels / 2;
    ga_uint32 numBytesRead;
    format.bitsPerSample = bytesPerSample * 8;
    format.numChannels = oggInfo->channels;
    format.sampleRate = oggInfo->rate;
    do{
      ga_int32 bitStream;
      ga_float32** samples;
      ga_int32 i;
      ga_int16* dst;
      ga_int32 samplesRead = ov_read_float(&oggFile, &samples, bufferSize, &bitStream);
      numBytesRead = samplesRead * oggInfo->channels * 2;
      data = gaX_cb->reallocFunc(data, totalBytes + numBytesRead);
      dst = (ga_int16*)(data + totalBytes);
      totalBytes += numBytesRead;
      for(i = 0; i < samplesRead; ++i)
      {
        ga_int32 channel;
        for(channel = 0; channel < oggInfo->channels; ++channel, ++dst)
          *dst = (ga_int16)(samples[channel][i] * 32768.0f);
      }
    } while (numBytesRead > 0);
  }
  ov_clear(&oggFile);
  fclose(rawFile);
  if(validFile)
  {
    ret = ga_sound_create(data, totalBytes, &format, 0);
    if(!ret)
      gaX_cb->freeFunc(data);
  }
  return ret;
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
  ga_Link tellJumps;
  /* WAV-Specific Data */
  union {
    struct {
      ga_WavData wavData;
    } wav;
    struct {
      OggVorbis_File oggFile;
      vorbis_info* oggInfo;
      gau_OggCallbackData oggCallbackData;
      ga_int32 pcmTotal;
    } ogg;
  };
} ga_StreamContext_File;

void gauX_sound_stream_file_produce(ga_HandleStream* in_stream);
void gauX_sound_stream_file_consume(ga_HandleStream* in_handle, ga_int32 in_samplesConsumed);
void gauX_sound_stream_file_seek(ga_HandleStream* in_handle, ga_int32 in_sampleOffset);
ga_int32 gauX_sound_stream_file_tell(ga_HandleStream* in_handle, ga_int32 in_param);
void gauX_sound_stream_file_destroy(ga_HandleStream* in_stream);

ga_Handle* gauX_stream_file_wav(ga_Mixer* in_mixer,
                                ga_int32 in_group,
                                ga_int32 in_bufferSize,
                                const char* in_filename,
                                ga_uint32 in_byteOffset,
                                ga_StreamContext_File* in_context)
{
  ga_Format fmt;
  ga_int32 validHdr;
  ga_StreamContext_File* context = in_context;
  validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &context->wav.wavData);
  if(validHdr != GA_SUCCESS)
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
                                ga_int32 in_group,
                                ga_int32 in_bufferSize,
                                const char* in_filename,
                                ga_uint32 in_byteOffset,
                                ga_StreamContext_File* in_context)
{
  ga_int32 endian = 0; // 0 is little endian (aka x86), 1 is big endian
  ga_int32 bytesPerSample = 2;
  ga_int32 totalBytes = 0;
  ga_Format fmt;
  ga_int32 validFile;
  ga_int32 retVal;
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
  in_context->ogg.pcmTotal = (ga_int32)ov_pcm_total(&in_context->ogg.oggFile, -1);
  validFile = in_context->ogg.oggInfo->channels <= 2;
  if(!validFile)
    return 0;
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
                           ga_int32 in_group,
                           ga_int32 in_bufferSize,
                           const char* in_filename,
                           ga_int32 in_fileFormat,
                           ga_uint32 in_byteOffset)
{
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
  ga_list_head(&context->tellJumps);
  if(context->file)
  {
    context->seekMutex = ga_mutex_create();
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
    ga_mutex_destroy(context->seekMutex);
  }

  if(context->file)
    fclose(context->file);
  gaX_cb->freeFunc(context->filename);
  gaX_cb->freeFunc(context);
  return 0;
}

ga_int32 gauX_buffer_read_wav_into_buffers(ga_CircBuffer* in_buffer, ga_int32 in_bytes,
                                           FILE* in_file)
{
  void* dataA;
  void* dataB;
  ga_uint32 sizeA;
  ga_uint32 sizeB;
  ga_int32 numBuffers;
  ga_CircBuffer* b = in_buffer;
  ga_int32 bytesToRead = in_bytes;
  numBuffers = ga_buffer_getFree(b, bytesToRead, &dataA, &sizeA, &dataB, &sizeB);
  if(numBuffers >= 1)
  {
    fread(dataA, 1, sizeA, in_file); /* TODO: Is this endian-safe? */
    if(numBuffers == 2)
      fread(dataB, 1, sizeB, in_file); /* TODO: Is this endian-safe? */
  }
  b->nextFree += bytesToRead;
  return bytesToRead;
}

ga_int32 gauX_read_oggvorbis(char* in_data, ga_int32 in_bytes,
                             OggVorbis_File* in_file, vorbis_info* in_info)
{
  ga_int32 samplesLeft = in_bytes / in_info->channels / 2;
  ga_uint32 numBytesRead;
  ga_int32 totalBytes = 0;
  do{
    ga_int32 bitStream;
    ga_float32** samples;
    ga_int32 i;
    ga_int16* dst;
    ga_int32 samplesRead = ov_read_float(in_file, &samples, samplesLeft, &bitStream);
    samplesLeft -= samplesRead;
    numBytesRead = samplesRead * in_info->channels * 2;
    dst = (ga_int16*)(in_data + totalBytes);
    totalBytes += numBytesRead;
    for(i = 0; i < samplesRead; ++i)
    {
      ga_int32 channel;
      for(channel = 0; channel < in_info->channels; ++channel, ++dst)
        *dst = (ga_int16)(samples[channel][i] * 32768.0f);
    }
  } while (numBytesRead > 0 && totalBytes <= in_bytes && samplesLeft);
  return totalBytes;
}

ga_int32 gauX_buffer_read_ogg_into_buffers(ga_CircBuffer* in_buffer, ga_int32 in_bytes,
                                       OggVorbis_File* in_file, vorbis_info* in_info)
{
  void* dataA;
  void* dataB;
  ga_uint32 sizeA = 0;
  ga_uint32 sizeB = 0;
  ga_int32 numBuffers;
  ga_int32 numWritten = 0;
  ga_CircBuffer* b = in_buffer;
  numBuffers = ga_buffer_getFree(b, in_bytes, &dataA, &sizeA, &dataB, &sizeB);
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
  ga_int32 pos;
  ga_int32 delta;
} ga_TellJumpData;

typedef struct ga_TellJumpLink {
  ga_Link link;
  ga_TellJumpData data;
} ga_TellJumpLink;

void gauX_sound_stream_pushTellJump(ga_Link* in_head, ga_int32 in_pos, ga_int32 in_delta)
{
  ga_TellJumpLink* link = gaX_cb->allocFunc(sizeof(ga_TellJumpLink));
  link->link.data = &link->data;
  link->data.pos = in_pos;
  link->data.delta = in_delta;
  ga_list_link(in_head->prev, (ga_Link*)link, &link->data);
}
ga_int32 gauX_sound_stream_peekTellJump(ga_Link* in_head, ga_int32* out_pos, ga_int32* out_delta)
{
  ga_TellJumpLink* link;
  if(in_head->next == in_head)
    return 0;
  link = (ga_TellJumpLink*)in_head->next;
  *out_pos = link->data.pos;
  *out_delta = link->data.delta;
  return 1;
}
ga_int32 gauX_sound_stream_popTellJump(ga_Link* in_head)
{
  ga_TellJumpLink* link;
  if(in_head->next == in_head)
    return 0;
  link = (ga_TellJumpLink*)in_head->next;
  ga_list_unlink((ga_Link*)link);
  gaX_cb->freeFunc(link);
  return 1;
}
ga_int32 gauX_sound_stream_processTellJumps(ga_Link* in_head, ga_int32 in_advance)
{
  ga_int32 ret = 0;
  ga_TellJumpLink* link = (ga_TellJumpLink*)in_head->next;
  while((ga_Link*)link != in_head)
  {
    ga_TellJumpLink* oldLink = link;
    link = (ga_TellJumpLink*)link->link.next;
    oldLink->data.pos -= in_advance;
    if(oldLink->data.pos <= 0)
    {
      ret += oldLink->data.delta;
      ga_list_unlink((ga_Link*)oldLink);
    }
  }
  return ret;
}
void gauX_sound_stream_clearTellJumps(ga_Link* in_head)
{
  while(gauX_sound_stream_popTellJump(in_head));
}

/* Complex */
void gauX_sound_stream_file_produce(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  ga_HandleStream* h = in_handle;
  ga_CircBuffer* b = h->buffer;
  ga_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
  ga_int32 bytesFree = ga_buffer_bytesFree(b);
  ga_int32 loop;
  ga_int32 loopStart;
  ga_int32 loopEnd;
  ga_mutex_lock(h->handleMutex);
  loop = h->loop;
  loopStart = h->loopStart;
  loopEnd = h->loopEnd;
  ga_mutex_unlock(h->handleMutex);

  if(context->seek >= 0)
  {
    ga_int32 samplePos;
    ga_mutex_lock(h->consumeMutex);
    ga_mutex_lock(context->seekMutex);
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
          ga_int32 totalSamples = context->wav.wavData.dataSize / sampleSize;
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
      ga_buffer_consume(h->buffer, ga_buffer_bytesAvail(h->buffer)); /* Clear buffer */
      gauX_sound_stream_clearTellJumps(&context->tellJumps); /* Clear tell-jump list */
    }
    ga_mutex_unlock(context->seekMutex);
    ga_mutex_unlock(h->consumeMutex);
  }

  while(bytesFree)
  {
    ga_int32 bytesWritten = 0;
    ga_int32 bytesToWrite =
      (loop && loopEnd > context->nextSample) ?
      (loopEnd - context->nextSample) * sampleSize:
      bytesFree;
    bytesToWrite = bytesToWrite > bytesFree ? bytesFree : bytesToWrite;
    switch(context->fileFormat)
    {
    case GA_FILE_FORMAT_WAV:
      {
        ga_int32 wavAvail = context->wav.wavData.dataSize - context->nextSample * sampleSize;
        ga_int32 bytesToRead = bytesToWrite > wavAvail ? wavAvail : bytesToWrite;
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
        ga_mutex_unlock(h->consumeMutex);
        ga_mutex_lock(context->seekMutex);
        /* Add tell jump */
        gauX_sound_stream_pushTellJump(
          &context->tellJumps,
          ga_buffer_bytesAvail(h->buffer) / sampleSize,
          loopStart - context->nextSample);
        ga_mutex_unlock(context->seekMutex);
        ga_mutex_unlock(h->consumeMutex);
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
void gauX_sound_stream_file_consume(ga_HandleStream* in_handle, ga_int32 in_samplesConsumed)
{
  ga_HandleStream* h = in_handle;
  ga_StreamContext_File* context = (ga_StreamContext_File*)h->streamContext;
  ga_int32 delta;
  ga_mutex_lock(context->seekMutex);
  context->tell += in_samplesConsumed;
  delta = gauX_sound_stream_processTellJumps(&context->tellJumps, in_samplesConsumed);
  context->tell += delta;
  ga_mutex_unlock(context->seekMutex);
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
  ga_int32 ret = -1;
  if(in_param == GA_TELL_PARAM_TOTAL)
  {
    switch(context->fileFormat)
    {
    case GA_FILE_FORMAT_WAV:
      {
        ga_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
        ret = context->wav.wavData.dataSize / sampleSize;
        break;
      }
    case GA_FILE_FORMAT_OGG:
      {
        ga_int32 sampleSize = ga_format_sampleSize(&in_handle->format);
        ga_int32 totalSamples = context->ogg.pcmTotal;
        ret = totalSamples;
        break;
      }
    }
  }
  else
  {
    ga_mutex_lock(context->seekMutex);
    ret = context->seek >= 0 ? context->seek : context->tell;
    ga_mutex_unlock(context->seekMutex);
  }
  return ret;
}
void gauX_sound_stream_file_destroy(ga_HandleStream* in_handle)
{
  ga_StreamContext_File* context = (ga_StreamContext_File*)in_handle->streamContext;
  ga_mutex_destroy(context->seekMutex);
  gauX_sound_stream_clearTellJumps(&context->tellJumps);
  gaX_cb->freeFunc(context->filename);
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
      fclose(context->file);
      break;
    }
  }
}
