#include "gorilla/ga.h"

#include "gorilla/ga_wav.h"
#include "gorilla/ga_openal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Version Functions */
gc_int32 ga_version_check(gc_int32 in_major, gc_int32 in_minor, gc_int32 in_rev)
{
  gc_int32 ret;
  ret = (in_major == GA_VERSION_MAJOR) ? 0 : (in_major < GA_VERSION_MAJOR) ? -1 : 1;
  if(ret == 0)
    ret = (in_minor == GA_VERSION_MINOR) ? 0 : (in_minor < GA_VERSION_MINOR) ? -1 : 1;
  if(ret == 0)
    ret = (in_rev == GA_VERSION_REV) ? 0 : (in_rev < GA_VERSION_REV) ? -1 : 1;
  return ret;
}

/* Format Functions */
gc_int32 ga_format_sampleSize(ga_Format* in_format)
{
  ga_Format* fmt = in_format;
  return (fmt->bitsPerSample >> 3) * fmt->numChannels;
}
gc_float32 ga_format_toSeconds(ga_Format* in_format, gc_int32 in_samples)
{
  return in_samples / (gc_float32)in_format->sampleRate;
}
gc_int32 ga_format_toSamples(ga_Format* in_format, gc_float32 in_seconds)
{
  return (gc_int32)(in_seconds * in_format->sampleRate);
}

/* Device Functions */
ga_Device* ga_device_open(int in_type, gc_int32 in_numBuffers)
{
  if(in_type == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    return (ga_Device*)gaX_device_open_openAl(in_numBuffers);
#else
    return 0;
#endif /* LINK_AGAINST_OPENAL */
  }
  else
    return 0;
}
gc_result ga_device_close(ga_Device* in_device)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    gaX_device_close_openAl(dev);
    return GC_SUCCESS;
#else
    return GC_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GC_ERROR_GENERIC;
}
gc_int32 ga_device_check(ga_Device* in_device)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    return gaX_device_check_openAl(dev);
#else
    return GC_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GC_ERROR_GENERIC;
}
gc_result ga_device_queue(ga_Device* in_device,
                         ga_Format* in_format,
                         gc_int32 in_numSamples,
                         void* in_buffer)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    return gaX_device_queue_openAl(dev, in_format, in_numSamples, in_buffer);
#else
    return GC_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GC_ERROR_GENERIC;
}

/* Sound Functions */
ga_Sound* ga_sound_create(void* in_data, gc_int32 in_size,
                          ga_Format* in_format, gc_int32 in_copy)
{
  ga_Sound* ret = gcX_ops->allocFunc(sizeof(ga_Sound));
  ret->isCopy = in_copy;
  ret->size = in_size;
  ret->loopStart = 0;
  ret->loopEnd = -1;
  ret->numSamples = in_size / ga_format_sampleSize(in_format);
  if(in_copy)
  {
    ret->data = gcX_ops->allocFunc(in_size);
    memcpy(ret->data, in_data, in_size);
  }
  else
    ret->data = in_data;
  memcpy(&ret->format, in_format, sizeof(ga_Format));
  return (ga_Sound*)ret;
}
gc_result ga_sound_setLoops(ga_Sound* in_sound,
                            gc_int32 in_loopStart, gc_int32 in_loopEnd)
{
  in_sound->loopStart = in_loopStart;
  in_sound->loopEnd = in_loopEnd;
  return GC_SUCCESS;
}
gc_int32 ga_sound_numSamples(ga_Sound* in_sound)
{
  return in_sound->size / ga_format_sampleSize(&in_sound->format);
}
gc_result ga_sound_destroy(ga_Sound* in_sound)
{
  if(in_sound->isCopy)
    gcX_ops->freeFunc(in_sound->data);
  gcX_ops->freeFunc(in_sound);
  return GC_SUCCESS;
}

/* Handle Functions */
void gaX_handle_init(ga_Handle* in_handle, ga_Mixer* in_mixer)
{
  ga_Handle* h = in_handle;
  h->state = GA_HANDLE_STATE_INITIAL;
  h->mixer = in_mixer;
  h->callback = 0;
  h->context = 0;
  h->gain = 1.0f;
  h->pitch = 1.0f;
  h->pan = 0.0f;
  h->loop = 0;
  h->handleMutex = gc_mutex_create();
}

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_Sound* in_sound)
{
  ga_HandleStatic* hs = (ga_HandleStatic*)gcX_ops->allocFunc(sizeof(ga_HandleStatic));
  ga_Handle* h = (ga_Handle*)hs;
  h->handleType = GA_HANDLE_TYPE_STATIC;
  hs->nextSample = 0;
  hs->sound = in_sound;
  hs->loopStart = in_sound->loopStart;
  hs->loopEnd = in_sound->loopEnd;
  gaX_handle_init(h, in_mixer);

  h->streamLink.next = 0;
  h->streamLink.prev = 0;

  gc_mutex_lock(in_mixer->mixMutex);
  gc_list_link(&in_mixer->mixList, &h->mixLink, h);
  gc_mutex_unlock(in_mixer->mixMutex);

  gc_mutex_lock(in_mixer->dispatchMutex);
  gc_list_link(&in_mixer->dispatchList, &h->dispatchLink, h);
  gc_mutex_unlock(in_mixer->dispatchMutex);

  return h;
}
ga_Handle* ga_handle_createStream(ga_Mixer* in_mixer,
                                  gc_int32 in_group,
                                  gc_int32 in_bufferSize,
                                  ga_Format* in_format,
                                  ga_StreamProduceFunc in_produceFunc,
                                  ga_StreamConsumeFunc in_consumeFunc,
                                  ga_StreamSeekFunc in_seekFunc,
                                  ga_StreamTellFunc in_tellFunc,
                                  ga_StreamDestroyFunc in_destroyFunc,
                                  void* in_context)
{
  ga_HandleStream* hs;
  ga_Handle* h;
  gc_CircBuffer* cb = gc_buffer_create(in_bufferSize);
  if(!cb)
    return 0;
  hs = (ga_HandleStream*)gcX_ops->allocFunc(sizeof(ga_HandleStream));
  h = (ga_Handle*)hs;
  h->handleType = GA_HANDLE_TYPE_STREAM;
  memcpy(&hs->format, in_format, sizeof(ga_Format));
  hs->buffer = cb;
  hs->group = in_group;
  hs->produceFunc = in_produceFunc;
  hs->consumeFunc = in_consumeFunc;
  hs->seekFunc = in_seekFunc;
  hs->tellFunc = in_tellFunc;
  hs->destroyFunc = in_destroyFunc;
  hs->streamContext = in_context;
  hs->loopStart = 0;
  hs->loopEnd = -1;
  hs->consumeMutex = gc_mutex_create();
  gaX_handle_init(h, in_mixer);

  gc_mutex_lock(in_mixer->streamMutex);
  gc_list_link(&in_mixer->streamList, &h->streamLink, h);
  gc_mutex_unlock(in_mixer->streamMutex);

  gc_mutex_lock(in_mixer->mixMutex);
  gc_list_link(&in_mixer->mixList, &h->mixLink, h);
  gc_mutex_unlock(in_mixer->mixMutex);

  gc_mutex_lock(in_mixer->dispatchMutex);
  gc_list_link(&in_mixer->dispatchList, &h->dispatchLink, h);
  gc_mutex_unlock(in_mixer->dispatchMutex);

  return h;
}
gc_result ga_handle_destroy(ga_Handle* in_handle)
{
  /* Sets the destroyed state. Will be cleaned up once all threads ACK. */
  gc_mutex_lock(in_handle->handleMutex);
  in_handle->state = GA_HANDLE_STATE_DESTROYED;
  gc_mutex_unlock(in_handle->handleMutex);
  return GC_SUCCESS;
}
gc_result gaX_handle_cleanup(ga_Handle* in_handle)
{
  /* May only be called from the dispatch thread */
  ga_Mixer* m = in_handle->mixer;
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* hs = (ga_HandleStatic*)in_handle;
      gc_mutex_destroy(hs->handleMutex);
      gcX_ops->freeFunc(hs);
      break;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* hs = (ga_HandleStream*)in_handle;
      gc_mutex_destroy(hs->handleMutex);
      gc_mutex_destroy(hs->consumeMutex);
      hs->destroyFunc(hs);
      gc_buffer_destroy(hs->buffer);
      gcX_ops->freeFunc(hs);
      break;
    }
  }
  return GC_SUCCESS;
}

gc_result ga_handle_play(ga_Handle* in_handle)
{
  gc_mutex_lock(in_handle->handleMutex);
  if(in_handle->state >= GA_HANDLE_STATE_FINISHED)
  {
    gc_mutex_unlock(in_handle->handleMutex);
    return GC_ERROR_GENERIC;
  }
  in_handle->state = GA_HANDLE_STATE_PLAYING;
  gc_mutex_unlock(in_handle->handleMutex);
  return GC_SUCCESS;
}
gc_result ga_handle_stop(ga_Handle* in_handle)
{
  gc_mutex_lock(in_handle->handleMutex);
  if(in_handle->state >= GA_HANDLE_STATE_FINISHED)
  {
    gc_mutex_unlock(in_handle->handleMutex);
    return GC_ERROR_GENERIC;
  }
  in_handle->state = GA_HANDLE_STATE_STOPPED;
  gc_mutex_unlock(in_handle->handleMutex);
  return GC_SUCCESS;
}
gc_int32 ga_handle_playing(ga_Handle* in_handle)
{
  return in_handle->state == GA_HANDLE_STATE_PLAYING ? GC_TRUE : GC_FALSE;
}
gc_int32 ga_handle_stopped(ga_Handle* in_handle)
{
  return in_handle->state == GA_HANDLE_STATE_STOPPED ? GC_TRUE : GC_FALSE;
}
gc_int32 ga_handle_finished(ga_Handle* in_handle)
{
  return in_handle->state >= GA_HANDLE_STATE_FINISHED ? GC_TRUE : GC_FALSE;
}
gc_int32 ga_handle_destroyed(ga_Handle* in_handle)
{
  return in_handle->state >= GA_HANDLE_STATE_DESTROYED ? GC_TRUE : GC_FALSE;
}
gc_result ga_handle_setCallback(ga_Handle* in_handle, ga_FinishCallback in_callback, void* in_context)
{
  /* Does not need mutex because it can only be called from the dispatch thread */
  in_handle->callback = in_callback;
  in_handle->context = in_context;
  return GC_SUCCESS;
}
gc_result ga_handle_setLoops(ga_Handle* in_handle,
                             gc_int32 in_loopStart, gc_int32 in_loopEnd)
{
  ga_Handle* h = in_handle;
  gc_mutex_lock(h->handleMutex);
  h->loopStart = in_loopStart;
  h->loopEnd = in_loopEnd;
  gc_mutex_unlock(h->handleMutex);
  return GC_SUCCESS;
}
gc_result ga_handle_setParamf(ga_Handle* in_handle, gc_int32 in_param,
                              gc_float32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN:
    gc_mutex_lock(h->handleMutex);
    h->gain = in_value;
    gc_mutex_unlock(h->handleMutex);
    return GC_SUCCESS;
  case GA_HANDLE_PARAM_PAN:
    gc_mutex_lock(h->handleMutex);
    h->pan = in_value;
    gc_mutex_unlock(h->handleMutex);
    return GC_SUCCESS;
  case GA_HANDLE_PARAM_PITCH:
    gc_mutex_lock(h->handleMutex);
    h->pitch = in_value;
    gc_mutex_unlock(h->handleMutex);
    return GC_SUCCESS;
  }
  return GC_ERROR_GENERIC;
}
gc_result ga_handle_getParamf(ga_Handle* in_handle, gc_int32 in_param,
                              gc_float32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: *out_value = h->gain; return GC_SUCCESS;
  case GA_HANDLE_PARAM_PAN: *out_value = h->pan; return GC_SUCCESS;
  case GA_HANDLE_PARAM_PITCH: *out_value = h->pitch; return GC_SUCCESS;
  }
  return GC_ERROR_GENERIC;
}
gc_result ga_handle_setParami(ga_Handle* in_handle, gc_int32 in_param,
                              gc_int32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_LOOP:
    gc_mutex_lock(h->handleMutex);
    h->loop = in_value;
    gc_mutex_unlock(h->handleMutex);
    return GC_SUCCESS;
  }
  return GC_ERROR_GENERIC;
}
gc_result ga_handle_getParami(ga_Handle* in_handle, gc_int32 in_param,
                              gc_int32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_LOOP: *out_value = h->loop; return GC_SUCCESS;
  }
  return GC_ERROR_GENERIC;
}
gc_result ga_handle_seek(ga_Handle* in_handle, gc_int32 in_sampleOffset)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* h = (ga_HandleStatic*)in_handle;
      if(in_sampleOffset >= ga_sound_numSamples(h->sound))
        in_sampleOffset = 0;
      gc_mutex_lock(h->handleMutex);
      h->nextSample = in_sampleOffset;
      gc_mutex_unlock(h->handleMutex);
      break;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* h = (ga_HandleStream*)in_handle;
      if(h->seekFunc)
        h->seekFunc(h, in_sampleOffset);
      break;
    }
  }
  return GC_SUCCESS;
}
gc_int32 ga_handle_tell(ga_Handle* in_handle, gc_int32 in_param)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* h = (ga_HandleStatic*)in_handle;
      if(in_param == GA_TELL_PARAM_CURRENT)
        return h->nextSample;
      else if(in_param == GA_TELL_PARAM_TOTAL)
        return h->sound->numSamples;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* h = (ga_HandleStream*)in_handle;
      if(h->tellFunc)
        return h->tellFunc(h, in_param);
    }
  }
  return -1;
}
ga_Format* ga_handle_format(ga_Handle* in_handle)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* h = (ga_HandleStatic*)in_handle;
      return &h->sound->format;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* h = (ga_HandleStream*)in_handle;
      return &h->format;
    }
  }
  return 0;
}

/* Mixer Functions */
ga_Mixer* ga_mixer_create(ga_Format* in_format, gc_int32 in_numSamples)
{
  ga_Mixer* ret = gcX_ops->allocFunc(sizeof(ga_Mixer));
  gc_int32 mixSampleSize;
  gc_list_head(&ret->dispatchList);
  gc_list_head(&ret->mixList);
  gc_list_head(&ret->streamList);
  ret->numSamples = in_numSamples;
  memcpy(&ret->format, in_format, sizeof(ga_Format));
  ret->mixFormat.bitsPerSample = 32;
  ret->mixFormat.numChannels = in_format->numChannels;
  ret->mixFormat.sampleRate = in_format->sampleRate;
  mixSampleSize = ga_format_sampleSize(&ret->mixFormat);
  ret->mixBuffer = (gc_int32*)gcX_ops->allocFunc(in_numSamples * mixSampleSize);
  ret->dispatchMutex = gc_mutex_create();
  ret->mixMutex = gc_mutex_create();
  ret->streamMutex = gc_mutex_create();
  return ret;
}
ga_Format* ga_mixer_format(ga_Mixer* in_mixer)
{
  return &in_mixer->format;
}
gc_int32 ga_mixer_numSamples(ga_Mixer* in_mixer)
{
  return in_mixer->numSamples;
}
gc_result ga_mixer_stream(ga_Mixer* in_mixer)
{
  ga_Mixer* m = in_mixer;
  gc_Link* link = m->streamList.next;
  while(link != &m->streamList)
  {
    ga_HandleStream* h = (ga_HandleStream*)link->data;
    ga_StreamProduceFunc produceFunc = h->produceFunc;
    gc_Link* oldLink = link;
    link = link->next;
    if(produceFunc)
      produceFunc(h);
    if(ga_handle_finished((ga_Handle*)h))
    {
      gc_mutex_lock(m->streamMutex);
      gc_list_unlink(oldLink);
      gc_mutex_unlock(m->streamMutex);
    }
  }
  return GC_SUCCESS;
}
void gaX_mixer_mix_buffer(ga_Mixer* in_mixer,
                          void* in_srcBufferA, gc_int32 in_srcBytesA,
                          void* in_srcBufferB, gc_int32 in_srcBytesB,
                          ga_Format* in_srcFormat,
                          gc_int32* in_dstBuffer, gc_int32 in_dstSamples,
                          gc_float32 in_gain, gc_float32 in_pan, gc_float32 in_pitch,
                          gc_int32* out_srcMixed, gc_int32* out_dstMixed)
{
  ga_Format* mixFmt = &in_mixer->format;
  gc_int32 mixerChannels = mixFmt->numChannels;
  gc_int32 srcChannels = in_srcFormat->numChannels;
  gc_float32 sampleScale = in_srcFormat->sampleRate / (gc_float32)mixFmt->sampleRate * in_pitch;
  gc_int32* dst = in_dstBuffer;
  gc_int32 numToFill = in_dstSamples;
  gc_float32 fj = 0.0f;
  gc_int32 j = 0;
  gc_int32 i = 0;
  gc_float32 pan = in_pan;
  gc_float32 gain = in_gain;
  gc_float32 srcSamplesRead = 0.0f;
  pan = (pan + 1.0f) / 2.0f;
  pan = pan > 1.0f ? 1.0f : pan;
  pan = pan < 0.0f ? 0.0f : pan;

  /* TODO: Support 8-bit/16-bit mono/stereo mixer format */
  switch(in_srcFormat->bitsPerSample)
  {
  case 16:
    {
      gc_int32 srcBytes = in_srcBytesA;
      const gc_int16* src = (const gc_int16*)in_srcBufferA;
      gc_int32 bufferB = GC_FALSE;
      while(i < numToFill * (gc_int32)mixerChannels && srcBytes >= 2 * srcChannels)
      {
        gc_int32 newJ, deltaSrcBytes;
        dst[i] += (gc_int32)((gc_int32)src[j] * gain * (1.0f - pan) * 2);
        dst[i + 1] += (gc_int32)((gc_int32)src[j + ((srcChannels == 1) ? 0 : 1)] * gain * pan * 2);
        i += mixerChannels;
        fj += sampleScale * srcChannels;
        srcSamplesRead += sampleScale * srcChannels;
        newJ = (gc_uint32)fj & (srcChannels == 1 ? ~0 : ~0x1);
        deltaSrcBytes = (newJ - j) * 2;
        j = newJ;
        if(deltaSrcBytes >= srcBytes && in_srcBytesB && bufferB != GC_TRUE)
        {
          gc_int32 overflow = deltaSrcBytes - srcBytes;
          srcBytes = in_srcBytesB - overflow;
          src = (gc_int16*)((char*)in_srcBufferB + overflow);
          j = overflow / 2;
          fj = fj - (gc_int32)fj;
          bufferB = GC_TRUE;
        }
        else
          srcBytes -= deltaSrcBytes;
      }
      break;
    }
  }
  if(srcSamplesRead * 2 > in_srcBytesA + in_srcBytesB)
    srcSamplesRead = (gc_float32)((in_srcBytesA + in_srcBytesB) / 2);
  *out_srcMixed = (gc_int32)srcSamplesRead / srcChannels;
  *out_dstMixed = i / mixerChannels;
}
void gaX_mixer_mix_static(ga_Mixer* in_mixer, ga_HandleStatic* in_handle)
{
  ga_HandleStatic* h = in_handle;

  /* Extract handle params */
  gc_int32 playing;
  gc_int32 endSample;
  gc_int32 loop;
  gc_int32 loopEnd;
  gc_int32 loopStart;
  gc_float32 gain;
  gc_float32 pan;
  gc_float32 pitch;
  gc_int32 nextSample;
  gc_int32 origNextSample;
  gc_mutex_lock(h->handleMutex);
  playing = h->state == GA_HANDLE_STATE_PLAYING;
  nextSample = origNextSample = h->nextSample;
  loop = h->loop;
  loopStart = h->loopStart;
  loopEnd = h->loopEnd;
  gain = h->gain;
  pan = h->pan;
  pitch = h->pitch;
  gc_mutex_unlock(h->handleMutex);

  if(playing)
  {
    ga_Mixer* m = in_mixer;
    ga_Sound* s = h->sound;
    gc_float32 sampleScale = s->format.sampleRate / (gc_float32)m->format.sampleRate * pitch;
    gc_int32 numSrcSamples = ga_sound_numSamples(s);
    gc_int32 srcSampleSize = ga_format_sampleSize(&s->format);
    gc_int32 dstSampleSize = ga_format_sampleSize(&m->format);
    gc_int32* dstBuffer = &m->mixBuffer[0];
    gc_int32 dstSamples = m->numSamples;
    gc_int32 dstSamplesScaled = (gc_int32)(dstSamples * sampleScale);

    /* Mix */
    loopEnd = loopEnd >= 0 ? loopEnd : numSrcSamples;
    endSample = (loop && loopEnd > nextSample) ? loopEnd : numSrcSamples;
    while(dstSamples > 0)
    {
      gc_int32 srcMixed, dstMixed;
      gaX_mixer_mix_buffer(in_mixer,
                           (char*)s->data + nextSample * srcSampleSize, (endSample - nextSample) * srcSampleSize,
                           0, 0, &s->format,
                           dstBuffer, dstSamples, gain, pan, pitch,
                           &srcMixed, &dstMixed);
      nextSample += srcMixed;
      dstSamples -= dstMixed;
      dstBuffer += dstMixed * m->mixFormat.numChannels;
      if(nextSample == endSample)
      {
        if(loop)
          nextSample = loopStart;
        else
        {
          /* Check/set the state atomically */
          gc_mutex_lock(h->handleMutex);
          if(h->state < GA_HANDLE_STATE_FINISHED)
            h->state = GA_HANDLE_STATE_FINISHED;
          if(h->nextSample == origNextSample)
            h->nextSample = nextSample;
          gc_mutex_unlock(h->handleMutex);
          return;
        }
      }
    }

    gc_mutex_lock(h->handleMutex);
    if(h->nextSample == origNextSample)
      h->nextSample = nextSample;
    gc_mutex_unlock(h->handleMutex);
  }
}
void gaX_mixer_mix_streaming(ga_Mixer* in_mixer, ga_HandleStream* in_handle, gc_int32* in_buffer, gc_int32 in_numSamples)
{
  ga_HandleStream* h = in_handle;
  ga_Mixer* m = in_mixer;
  gc_int32 streamFinished = !h->produceFunc;
  if(streamFinished && gc_buffer_bytesAvail(h->buffer) == 0)
  {
    /* Stream is finished! */
    gc_mutex_lock(h->handleMutex);
    if(h->state < GA_HANDLE_STATE_FINISHED)
      h->state = GA_HANDLE_STATE_FINISHED;
    gc_mutex_unlock(h->handleMutex);
    return;
  }
  else
  {
    if(h->state == GA_HANDLE_STATE_PLAYING)
    {
      /* Check if we have enough samples to stream a full buffer */
      gc_int32 srcSampleSize = ga_format_sampleSize(&h->format);
      gc_int32 dstSampleSize = ga_format_sampleSize(&m->format);
      gc_float32 oldPitch = h->pitch;
      gc_float32 dstToSrc = h->format.sampleRate / (gc_float32)m->format.sampleRate * oldPitch;
      gc_int32 avail = gc_buffer_bytesAvail(h->buffer);
      gc_int32 availSrcSamples = avail / srcSampleSize;
      gc_int32 availDstSamples = (gc_int32)(availSrcSamples / dstToSrc);
      if(availDstSamples > in_numSamples || (avail && streamFinished))
      {
        gc_float32 gain, pan, pitch;
        gc_int32* dstBuffer;
        gc_int32 dstSamples;

        gc_mutex_lock(h->handleMutex);
        gain = h->gain;
        pan = h->pan;
        pitch = h->pitch;
        gc_mutex_unlock(h->handleMutex);

        /* We avoided a mutex lock by using pitch to check if buffer has enough dst samples */
        /* If it has changed since then, we re-test to make sure we still have enough samples */
        if(oldPitch != pitch)
        {
          dstToSrc = h->format.sampleRate / (gc_float32)m->format.sampleRate * pitch;
          availDstSamples = (gc_int32)(availSrcSamples / dstToSrc);
          if(availDstSamples <= in_numSamples)
            return;
        }

        /* TODO: Pull as much out of this mutexed region as possible */
        gc_mutex_lock(h->consumeMutex);
        dstBuffer = &m->mixBuffer[0];
        dstSamples = in_numSamples;
        if(dstSamples > 0)
        {
          void* dataA;
          void* dataB;
          gc_uint32 sizeA, sizeB;
          gc_int32 dstBytes = dstSamples * dstSampleSize;
          gc_int32 numBuffers = gc_buffer_getAvail(h->buffer, avail,
                                                   &dataA, &sizeA, &dataB, &sizeB);
          gc_int32 srcMixed = 0, dstMixed = 0;
          if(numBuffers >= 1)
          {
            if(numBuffers == 1)
            {
              dataB = 0;
              sizeB = 0;
            }
            gaX_mixer_mix_buffer(in_mixer, dataA, sizeA, dataB, sizeB, &h->format,
                                 dstBuffer, dstSamples,
                                 gain, pan, pitch,
                                 &srcMixed, &dstMixed);
          }
          gc_buffer_consume(h->buffer, srcMixed * srcSampleSize);
          if(h->consumeFunc)
            h->consumeFunc(h, srcMixed);
          dstSamples -= dstMixed;
        }
        gc_mutex_unlock(h->consumeMutex);
      }
    }
  }
}
gc_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer)
{
  gc_int32 i;
  ga_Mixer* m = in_mixer;
  gc_Link* link;
  gc_int32 end = m->numSamples * m->format.numChannels;
  ga_Format* fmt = &m->format;
  gc_int32 mixSampleSize = ga_format_sampleSize(&m->mixFormat);
  memset(&m->mixBuffer[0], 0, m->numSamples * mixSampleSize);

  link = m->mixList.next;
  while(link != &m->mixList)
  {
    ga_Handle* h = (ga_Handle*)link->data;
    gc_Link* oldLink = link;
    link = link->next;
    switch(h->handleType)
    {
    case GA_HANDLE_TYPE_STATIC:
      {
        gaX_mixer_mix_static(m, (ga_HandleStatic*)h);
        break;
      }
    case GA_HANDLE_TYPE_STREAM:
      {
        gaX_mixer_mix_streaming(m, (ga_HandleStream*)h, &m->mixBuffer[0], m->numSamples);
        break;
      }
    }
    if(ga_handle_finished(h))
    {
      gc_mutex_lock(m->mixMutex);
      gc_list_unlink(oldLink);
      gc_mutex_unlock(m->mixMutex);
    }
  }

  switch(fmt->bitsPerSample) /* mixBuffer will already be correct bps */
  {
  case 8:
    {
      gc_int8* mix = (gc_int8*)out_buffer;
      for(i = 0; i < end; ++i)
      {
        gc_int32 sample = m->mixBuffer[i];
        mix[i] = (gc_int8)(sample > -128 ? (sample < 127 ? sample : 127) : -128);
      }
      break;
    }
  case 16:
    {
      gc_int16* mix = (gc_int16*)out_buffer;
      for(i = 0; i < end; ++i)
      {
        gc_int32 sample = m->mixBuffer[i];
        mix[i] = (gc_int16)(sample > -32768 ? (sample < 32767 ? sample : 32767) : -32768);
      }
      break;
    }
  }
  return GC_SUCCESS;
}
gc_result ga_mixer_dispatch(ga_Mixer* in_mixer)
{
  ga_Mixer* m = in_mixer;
  gc_Link* link = m->dispatchList.next;
  while(link != &m->dispatchList)
  {
    gc_Link* oldLink = link;
    ga_Handle* oldHandle = (ga_Handle*)oldLink->data;
    link = link->next;

    /* Remove finished handles and call callbacks */
    if(ga_handle_destroyed(oldHandle))
    {
      if(!oldHandle->mixLink.next && !oldHandle->streamLink.next)
      {
        /* NOTES ABOUT THREADING POLICY WITH REGARD TO LINKED LISTS: */
        /* Only a single thread may iterate through any list */
        /* The thread that unlinks must be the only thread that iterates through the list */
        /* A single auxiliary thread may link(), but must mutex-lock to avoid link/unlink collisions */
        gc_mutex_lock(m->dispatchMutex);
        gc_list_unlink(&oldHandle->dispatchLink);
        gc_mutex_unlock(m->dispatchMutex);
        gaX_handle_cleanup(oldHandle);
      }
    }
    else if(oldHandle->callback && ga_handle_finished(oldHandle))
    {
      oldHandle->callback(oldHandle, oldHandle->context);
      oldHandle->callback = 0;
      oldHandle->context = 0;
    }
  }
  return GC_SUCCESS;
}
gc_result ga_mixer_destroy(ga_Mixer* in_mixer)
{
  /* NOTE: Mixer/handles must no longer be in use on any thread when destroy is called */
  ga_Mixer* m = in_mixer;
  gc_Link* link;
  link = m->dispatchList.next;
  while(link != &m->dispatchList)
  {
    ga_Handle* oldHandle = (ga_Handle*)link->data;
    link = link->next;
    gaX_handle_cleanup(oldHandle);
  }

  gc_mutex_destroy(in_mixer->dispatchMutex);
  gc_mutex_destroy(in_mixer->mixMutex);
  gc_mutex_destroy(in_mixer->streamMutex);

  gcX_ops->freeFunc(in_mixer->mixBuffer);
  gcX_ops->freeFunc(in_mixer);
  return GC_SUCCESS;
}
