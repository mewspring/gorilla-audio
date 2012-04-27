#include "gorilla/ga.h"

#include "gorilla/ga_wav.h"
#include "gorilla/ga_openal.h"
#include "gorilla/ga_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* System Functions */
ga_SystemOps* gaX_cb = 0;

static void* gaX_defaultAllocFunc(ga_uint32 in_size)
{
  return malloc(in_size);
}
static void* gaX_defaultReallocFunc(void* in_ptr, ga_uint32 in_size)
{
  return realloc(in_ptr, in_size);
}
static void gaX_defaultFreeFunc(void* in_ptr)
{
  free(in_ptr);
}
static ga_SystemOps s_defaultCallbacks;
ga_result ga_initialize(ga_SystemOps* in_callbacks)
{
  s_defaultCallbacks.allocFunc = &gaX_defaultAllocFunc;
  s_defaultCallbacks.reallocFunc = &gaX_defaultReallocFunc;
  s_defaultCallbacks.freeFunc = &gaX_defaultFreeFunc;
  gaX_cb = in_callbacks ? in_callbacks : &s_defaultCallbacks;
  return GA_SUCCESS;
}
ga_result ga_shutdown()
{
  gaX_cb = 0;
  return GA_SUCCESS;
}

/* Format Functions */
ga_int32 ga_format_sampleSize(ga_Format* in_format)
{
  ga_Format* fmt = in_format;
  return (fmt->bitsPerSample >> 3) * fmt->numChannels;
}
ga_float32 ga_format_toSeconds(ga_Format* in_format, ga_int32 in_samples)
{
  return in_samples / (ga_float32)in_format->sampleRate;
}
ga_int32 ga_format_toSamples(ga_Format* in_format, ga_float32 in_seconds)
{
  return (ga_int32)(in_seconds * in_format->sampleRate);
}

/* Device Functions */
ga_Device* ga_device_open(int in_type)
{
  if(in_type == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    return (ga_Device*)gaX_device_open_openAl();
#else
    return 0;
#endif /* LINK_AGAINST_OPENAL */
  }
  else
    return 0;
}
ga_result ga_device_close(ga_Device* in_device)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    gaX_device_close_openAl(dev);
    return GA_SUCCESS;
#else
    return GA_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GA_ERROR_GENERIC;
}
ga_int32 ga_device_check(ga_Device* in_device)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    return gaX_device_check_openAl(dev);
#else
    return GA_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GA_ERROR_GENERIC;
}
ga_result ga_device_queue(ga_Device* in_device,
                         ga_Format* in_format,
                         ga_int32 in_numSamples,
                         void* in_buffer)
{
  if(in_device->devType == GA_DEVICE_TYPE_OPENAL)
  {
#ifdef LINK_AGAINST_OPENAL
    ga_DeviceImpl_OpenAl* dev = (ga_DeviceImpl_OpenAl*)in_device;
    return gaX_device_queue_openAl(dev, in_format, in_numSamples, in_buffer);
#else
    return GA_ERROR_GENERIC;
#endif /* LINK_AGAINST_OPENAL */
  }
  return GA_ERROR_GENERIC;
}

/* Sound Functions */
ga_Sound* ga_sound_create(void* in_data, ga_int32 in_size,
                          ga_Format* in_format, ga_int32 in_copy)
{
  ga_Sound* ret = gaX_cb->allocFunc(sizeof(ga_Sound));
  ret->isCopy = in_copy;
  ret->size = in_size;
  ret->loopStart = 0;
  ret->loopEnd = -1;
  ret->numSamples = in_size / ga_format_sampleSize(in_format);
  if(in_copy)
  {
    ret->data = gaX_cb->allocFunc(in_size);
    memcpy(ret->data, in_data, in_size);
  }
  else
    ret->data = in_data;
  memcpy(&ret->format, in_format, sizeof(ga_Format));
  return (ga_Sound*)ret;
}
ga_result ga_sound_setLoops(ga_Sound* in_sound,
                            ga_int32 in_loopStart, ga_int32 in_loopEnd)
{
  in_sound->loopStart = in_loopStart;
  in_sound->loopEnd = in_loopEnd;
  return GA_SUCCESS;
}
ga_int32 ga_sound_numSamples(ga_Sound* in_sound)
{
  return in_sound->size / ga_format_sampleSize(&in_sound->format);
}
ga_result ga_sound_destroy(ga_Sound* in_sound)
{
  if(in_sound->isCopy)
    gaX_cb->freeFunc(in_sound->data);
  gaX_cb->freeFunc(in_sound);
  return GA_SUCCESS;
}

/* List Functions */
void ga_list_head(ga_Link* in_head)
{
  in_head->next = in_head;
  in_head->prev = in_head;
}
void ga_list_link(ga_Link* in_head, ga_Link* in_link, void* in_data)
{
  in_link->data = in_data;
  in_link->prev = in_head;
  in_link->next = in_head->next;
  in_head->next->prev = in_link;
  in_head->next = in_link;
}
void ga_list_unlink(ga_Link* in_link)
{
  in_link->prev->next = in_link->next;
  in_link->next->prev = in_link->prev;
  in_link->prev = 0;
  in_link->next = 0;
  in_link->data = 0;
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
  h->envDuration = -1;
  h->envGainA = 1.0f;
  h->envGainB = 1.0f;
  h->handleMutex = ga_mutex_create();
}

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_Sound* in_sound)
{
  ga_HandleStatic* hs = (ga_HandleStatic*)gaX_cb->allocFunc(sizeof(ga_HandleStatic));
  ga_Handle* h = (ga_Handle*)hs;
  h->handleType = GA_HANDLE_TYPE_STATIC;
  hs->nextSample = 0;
  hs->sound = in_sound;
  hs->loopStart = in_sound->loopStart;
  hs->loopEnd = in_sound->loopEnd;
  gaX_handle_init(h, in_mixer);

  h->streamLink.next = 0;
  h->streamLink.prev = 0;

  ga_mutex_lock(in_mixer->mixMutex);
  ga_list_link(&in_mixer->mixList, &h->mixLink, h);
  ga_mutex_unlock(in_mixer->mixMutex);

  ga_mutex_lock(in_mixer->dispatchMutex);
  ga_list_link(&in_mixer->dispatchList, &h->dispatchLink, h);
  ga_mutex_unlock(in_mixer->dispatchMutex);

  return h;
}
ga_Handle* ga_handle_createStream(ga_Mixer* in_mixer,
                                  ga_int32 in_group,
                                  ga_int32 in_bufferSize,
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
  ga_CircBuffer* cb = ga_buffer_create(in_bufferSize);
  if(!cb)
    return 0;
  hs = (ga_HandleStream*)gaX_cb->allocFunc(sizeof(ga_HandleStream));
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
  hs->consumeMutex = ga_mutex_create();
  gaX_handle_init(h, in_mixer);

  ga_mutex_lock(in_mixer->streamMutex);
  ga_list_link(&in_mixer->streamList, &h->streamLink, h);
  ga_mutex_unlock(in_mixer->streamMutex);

  ga_mutex_lock(in_mixer->mixMutex);
  ga_list_link(&in_mixer->mixList, &h->mixLink, h);
  ga_mutex_unlock(in_mixer->mixMutex);

  ga_mutex_lock(in_mixer->dispatchMutex);
  ga_list_link(&in_mixer->dispatchList, &h->dispatchLink, h);
  ga_mutex_unlock(in_mixer->dispatchMutex);

  return h;
}
ga_result ga_handle_destroy(ga_Handle* in_handle)
{
  /* Sets the destroyed state. Will be cleaned up once all threads ACK. */
  ga_mutex_lock(in_handle->handleMutex);
  in_handle->state = GA_HANDLE_STATE_DESTROYED;
  ga_mutex_unlock(in_handle->handleMutex);
  return GA_SUCCESS;
}
ga_result gaX_handle_cleanup(ga_Handle* in_handle)
{
  /* May only be called from the dispatch thread */
  ga_Mixer* m = in_handle->mixer;
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* hs = (ga_HandleStatic*)in_handle;
      ga_mutex_destroy(hs->handleMutex);
      gaX_cb->freeFunc(hs);
      break;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* hs = (ga_HandleStream*)in_handle;
      ga_mutex_destroy(hs->handleMutex);
      ga_mutex_destroy(hs->consumeMutex);
      hs->destroyFunc(hs);
      ga_buffer_destroy(hs->buffer);
      gaX_cb->freeFunc(hs);
      break;
    }
  }
  return GA_SUCCESS;
}

ga_result ga_handle_play(ga_Handle* in_handle)
{
  ga_mutex_lock(in_handle->handleMutex);
  if(in_handle->state >= GA_HANDLE_STATE_FINISHED)
  {
    ga_mutex_unlock(in_handle->handleMutex);
    return GA_ERROR_GENERIC;
  }
  in_handle->state = GA_HANDLE_STATE_PLAYING;
  ga_mutex_unlock(in_handle->handleMutex);
  return GA_SUCCESS;
}
ga_result ga_handle_stop(ga_Handle* in_handle)
{
  ga_mutex_lock(in_handle->handleMutex);
  if(in_handle->state >= GA_HANDLE_STATE_FINISHED)
  {
    ga_mutex_unlock(in_handle->handleMutex);
    return GA_ERROR_GENERIC;
  }
  in_handle->state = GA_HANDLE_STATE_STOPPED;
  ga_mutex_unlock(in_handle->handleMutex);
  return GA_SUCCESS;
}
ga_int32 ga_handle_finished(ga_Handle* in_handle)
{
  return in_handle->state >= GA_HANDLE_STATE_FINISHED ? GA_TRUE : GA_FALSE;
}
ga_int32 ga_handle_destroyed(ga_Handle* in_handle)
{
  return in_handle->state >= GA_HANDLE_STATE_DESTROYED ? GA_TRUE : GA_FALSE;
}
ga_result ga_handle_setCallback(ga_Handle* in_handle, ga_FinishCallback in_callback, void* in_context)
{
  /* Does not need mutex because it can only be called from the dispatch thread */
  in_handle->callback = in_callback;
  in_handle->context = in_context;
  return GA_SUCCESS;
}
ga_result ga_handle_setLoops(ga_Handle* in_handle,
                             ga_int32 in_loopStart, ga_int32 in_loopEnd)
{
  ga_Handle* h = in_handle;
  ga_mutex_lock(h->handleMutex);
  h->loopStart = in_loopStart;
  h->loopEnd = in_loopEnd;
  ga_mutex_unlock(h->handleMutex);
  return GA_SUCCESS;
}
ga_result ga_handle_setParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN:
    ga_mutex_lock(h->handleMutex);
    h->envDuration = -1;
    h->gain = in_value;
    ga_mutex_unlock(h->handleMutex);
    return GA_SUCCESS;
  case GA_HANDLE_PARAM_PAN:
    ga_mutex_lock(h->handleMutex);
    h->pan = in_value;
    ga_mutex_unlock(h->handleMutex);
    return GA_SUCCESS;
  case GA_HANDLE_PARAM_PITCH:
    ga_mutex_lock(h->handleMutex);
    h->pitch = in_value;
    ga_mutex_unlock(h->handleMutex);
    return GA_SUCCESS;
  }
  return GA_ERROR_GENERIC;
}
ga_result ga_handle_getParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: *out_value = h->gain; return GA_SUCCESS;
  case GA_HANDLE_PARAM_PAN: *out_value = h->pan; return GA_SUCCESS;
  case GA_HANDLE_PARAM_PITCH: *out_value = h->pitch; return GA_SUCCESS;
  }
  return GA_ERROR_GENERIC;
}
ga_result ga_handle_setParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_LOOP:
    ga_mutex_lock(h->handleMutex);
    h->loop = in_value;
    ga_mutex_unlock(h->handleMutex);
    return GA_SUCCESS;
  }
  return GA_ERROR_GENERIC;
}
ga_result ga_handle_getParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_LOOP: *out_value = h->loop; return GA_SUCCESS;
  }
  return GA_ERROR_GENERIC;
}
ga_result ga_handle_seek(ga_Handle* in_handle, ga_int32 in_sampleOffset)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* h = (ga_HandleStatic*)in_handle;
      if(in_sampleOffset >= ga_sound_numSamples(h->sound))
        in_sampleOffset = 0;
      ga_mutex_lock(h->handleMutex);
      h->nextSample = in_sampleOffset;
      ga_mutex_unlock(h->handleMutex);
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
  return GA_SUCCESS;
}
ga_int32 ga_handle_tell(ga_Handle* in_handle, ga_int32 in_param)
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

ga_result ga_handle_envelope(ga_Handle* in_handle, ga_int32 in_duration,
                             ga_float32 in_gain)
{
  ga_mutex_lock(in_handle->handleMutex);
  in_handle->envDuration = in_duration;
  in_handle->envGainA = in_handle->gain;
  in_handle->envGainB = in_gain;
  ga_mutex_unlock(in_handle->handleMutex);
  return GA_SUCCESS;
}

/* Mixer Functions */
ga_Mixer* ga_mixer_create(ga_Format* in_format, ga_int32 in_numSamples)
{
  ga_Mixer* ret = gaX_cb->allocFunc(sizeof(ga_Mixer));
  ga_int32 mixSampleSize;
  ga_list_head(&ret->dispatchList);
  ga_list_head(&ret->mixList);
  ga_list_head(&ret->streamList);
  ret->numSamples = in_numSamples;
  memcpy(&ret->format, in_format, sizeof(ga_Format));
  ret->mixFormat.bitsPerSample = 32;
  ret->mixFormat.numChannels = in_format->numChannels;
  ret->mixFormat.sampleRate = in_format->sampleRate;
  mixSampleSize = ga_format_sampleSize(&ret->mixFormat);
  ret->mixBuffer = (ga_int32*)gaX_cb->allocFunc(in_numSamples * mixSampleSize);
  ret->dispatchMutex = ga_mutex_create();
  ret->mixMutex = ga_mutex_create();
  ret->streamMutex = ga_mutex_create();
  return ret;
}
ga_result ga_mixer_stream(ga_Mixer* in_mixer)
{
  ga_Mixer* m = in_mixer;
  ga_Link* link = m->streamList.next;
  while(link != &m->streamList)
  {
    ga_HandleStream* h = (ga_HandleStream*)link->data;
    ga_StreamProduceFunc produceFunc = h->produceFunc;
    ga_Link* oldLink = link;
    link = link->next;
    if(produceFunc)
      produceFunc(h);
    if(ga_handle_finished((ga_Handle*)h))
    {
      ga_mutex_lock(m->streamMutex);
      ga_list_unlink(oldLink);
      ga_mutex_unlock(m->streamMutex);
    }
  }
  return GA_SUCCESS;
}
ga_int32 gaX_mixer_mix_buffer(ga_Mixer* in_mixer,
                              void* in_srcBuffer, ga_int32 in_srcSamples, ga_Format* in_srcFormat,
                              ga_int32* in_dstBuffer, ga_int32 in_dstSamples,
                              ga_float32 in_gain, ga_float32 in_pan, ga_float32 in_pitch)
{
  ga_Format* mixFmt = &in_mixer->format;
  ga_int32 mixerChannels = mixFmt->numChannels;
  ga_int32 srcChannels = in_srcFormat->numChannels;
  ga_int32 numSamples = in_srcSamples;
  ga_float32 sampleScale = in_srcFormat->sampleRate / (ga_float32)mixFmt->sampleRate * in_pitch;
  ga_int32* dst = in_dstBuffer;
  ga_int32 numToFill = in_dstSamples;
  ga_float32 fj = 0.0f;
  ga_int32 j = 0;
  ga_int32 i = 0;
  ga_int32 available;
  ga_float32 pan = in_pan;
  ga_float32 gain = in_gain;

  available = in_srcSamples;

  pan = (pan + 1.0f) / 2.0f;
  pan = pan > 1.0f ? 1.0f : pan;
  pan = pan < 0.0f ? 0.0f : pan;

  /* TODO: Support stereo mixer format */
  switch(in_srcFormat->bitsPerSample)
  {
  case 8:
    {
      const ga_uint8* src = (const ga_uint8*)in_srcBuffer;
      while(i < numToFill * mixerChannels && j < available * srcChannels)
      {
        dst[i] += (ga_int32)(((ga_int32)src[j] - 128) * gain * (1.0f - pan) * 256.0f * 2);
        dst[i + 1] += (ga_int32)(((ga_int32)src[j + ((srcChannels == 1) ? 0 : 1)] - 128) * gain * pan * 256.0f * 2);
        fj += sampleScale * srcChannels;
        j = (ga_uint32)fj;
        j = j & ((srcChannels == 1) ? ~0 : ~0x1);
        j = j > available * srcChannels ? available * srcChannels : j;
        i += mixerChannels;
      }
      break;
    }
  case 16:
    {
      const ga_int16* src = (const ga_int16*)in_srcBuffer;
      while(i < numToFill * (ga_int32)mixerChannels && j < available * srcChannels)
      {
        dst[i] += (ga_int32)((ga_int32)src[j] * gain * (1.0f - pan) * 2);
        dst[i + 1] += (ga_int32)((ga_int32)src[j + ((srcChannels == 1) ? 0 : 1)] * gain * pan * 2);
        fj += sampleScale * srcChannels;
        j = (ga_uint32)fj;
        j = j & (srcChannels == 1 ? ~0 : ~0x1);
        j = j > available * srcChannels ? available * srcChannels : j;
        i += mixerChannels;
      }
      break;
    }
  }
  return i > j ? i : j; /* TODO: This is incorrect. Needs to be fixed to get pitch working. */
}
void gaX_mixer_mix_static(ga_Mixer* in_mixer, ga_HandleStatic* in_handle)
{
  ga_HandleStatic* h = in_handle;

  /* Extract handle params */
  ga_int32 playing;
  ga_int32 numToMix;
  ga_int32 endSample;
  ga_int32 loop;
  ga_int32 loopEnd;
  ga_int32 loopStart;
  ga_float32 gain;
  ga_float32 pan;
  ga_float32 pitch;
  ga_int32 nextSample;
  ga_int32 origNextSample;
  ga_mutex_lock(h->handleMutex);
  playing = h->state == GA_HANDLE_STATE_PLAYING;
  nextSample = origNextSample = h->nextSample;
  loop = h->loop;
  loopStart = h->loopStart;
  loopEnd = h->loopEnd;
  gain = h->gain;
  pan = h->pan;
  pitch = h->pitch;
  ga_mutex_unlock(h->handleMutex);

  if(playing)
  {
    ga_Mixer* m = in_mixer;
    ga_Sound* s = h->sound;
    ga_int32 numSrcSamples = ga_sound_numSamples(s);
    ga_int32 srcSampleSize = ga_format_sampleSize(&s->format);
    ga_int32 dstSampleSize = ga_format_sampleSize(&m->format);
    ga_int32* dstBuffer = &m->mixBuffer[0];
    ga_int32 dstSamples = m->numSamples;

    /* Mix */
    loopEnd = loopEnd >= 0 ? loopEnd : numSrcSamples;
    endSample = (loop && loopEnd > nextSample) ? loopEnd : numSrcSamples;
    numToMix = endSample - nextSample;
    numToMix = numToMix > dstSamples ? dstSamples : numToMix;
    while(numToMix > 0)
    {
      gaX_mixer_mix_buffer(in_mixer,
                           (char*)s->data + nextSample * srcSampleSize,
                           numToMix, &s->format,
                           dstBuffer, dstSamples, gain, pan, pitch);
      nextSample += numToMix;
      dstSamples -= numToMix;
      dstBuffer += numToMix * m->mixFormat.numChannels;
      numToMix = 0;
      if(nextSample == endSample)
      {
        if(loop)
        {
          nextSample = loopStart;
          numToMix = loopEnd - nextSample;
          numToMix = numToMix > dstSamples ? dstSamples : numToMix;
        }
        else
        {
          /* Check/set the state atomically */
          ga_mutex_lock(h->handleMutex);
          if(h->state < GA_HANDLE_STATE_FINISHED)
            h->state = GA_HANDLE_STATE_FINISHED;
          if(h->nextSample == origNextSample)
            h->nextSample = nextSample;
          ga_mutex_unlock(h->handleMutex);
          return;
        }
      }
    }

    ga_mutex_lock(h->handleMutex);
    if(h->nextSample == origNextSample)
      h->nextSample = nextSample;
    ga_mutex_unlock(h->handleMutex);
  }
}
void gaX_mixer_mix_streaming(ga_Mixer* in_mixer, ga_HandleStream* in_handle, ga_int32* in_buffer, ga_int32 in_numSamples)
{
  ga_HandleStream* h = in_handle;
  ga_Mixer* m = in_mixer;
  ga_int32 avail = (ga_int32)ga_buffer_bytesAvail(h->buffer);
  ga_int32 streamFinished = !h->produceFunc;
  if(avail >= m->numSamples || streamFinished)
  {
    if(avail > 0 && h->state == GA_HANDLE_STATE_PLAYING)
    {
      ga_float32 gain;
      ga_float32 pan;
      ga_float32 pitch;
      ga_int32 srcSampleSize;
      ga_int32 dstSampleSize;
      ga_int32* dstBuffer;
      ga_int32 dstSamples;
      ga_int32 numSrcSamples;
      ga_int32 numToMix;

      ga_mutex_lock(h->handleMutex);
      gain = h->gain;
      pan = h->pan;
      pitch = h->pitch;
      ga_mutex_unlock(h->handleMutex);

      ga_mutex_lock(h->consumeMutex);
      srcSampleSize = ga_format_sampleSize(&h->format);
      dstSampleSize = ga_format_sampleSize(&m->format);
      dstBuffer = &m->mixBuffer[0];
      dstSamples = m->numSamples;
      numSrcSamples = ga_buffer_bytesAvail(h->buffer) / ga_format_sampleSize(&h->format);
      numToMix = numSrcSamples > dstSamples ? dstSamples : numSrcSamples;
      if(numToMix > 0)
      {
        void* dataA;
        void* dataB;
        ga_uint32 sizeA;
        ga_uint32 sizeB;
        ga_int32 bytesToMix = numToMix * srcSampleSize;
        ga_int32 numBuffers = ga_buffer_getAvail(h->buffer, bytesToMix,
                                                 &dataA, &sizeA, &dataB, &sizeB);
        if(numBuffers >= 1)
        {
          gaX_mixer_mix_buffer(in_mixer, dataA,
                               sizeA / srcSampleSize, &h->format,
                               dstBuffer, sizeA / srcSampleSize,
                               gain, pan, pitch);
          if(numBuffers == 2)
          {
            dstSamples -= sizeA / srcSampleSize;
            dstBuffer += sizeA;
            gaX_mixer_mix_buffer(in_mixer, dataB,
                                 sizeB / srcSampleSize, &h->format,
                                 dstBuffer, sizeB / srcSampleSize,
                                 gain, pan, pitch);
          }
        }
        ga_buffer_consume(h->buffer, bytesToMix);
        if(h->consumeFunc)
          h->consumeFunc(h, numToMix);
      }
      ga_mutex_unlock(h->consumeMutex);
    }
    else if(streamFinished)
    {
      /* Stream is finished! */
      ga_mutex_lock(h->handleMutex);
      if(h->state < GA_HANDLE_STATE_FINISHED)
        h->state = GA_HANDLE_STATE_FINISHED;
      ga_mutex_unlock(h->handleMutex);
      return;
    }
  }
}
ga_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer)
{
  ga_int32 i;
  ga_Mixer* m = in_mixer;
  ga_Link* link;
  ga_int32 end = m->numSamples * m->format.numChannels;
  ga_Format* fmt = &m->format;
  ga_int32 mixSampleSize = ga_format_sampleSize(&m->mixFormat);
  memset(&m->mixBuffer[0], 0, m->numSamples * mixSampleSize);

  link = m->mixList.next;
  while(link != &m->mixList)
  {
    ga_Handle* h = (ga_Handle*)link->data;
    ga_Link* oldLink = link;
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
      ga_mutex_lock(m->mixMutex);
      ga_list_unlink(oldLink);
      ga_mutex_unlock(m->mixMutex);
    }
  }

  switch(fmt->bitsPerSample) /* mixBuffer will already be correct bps */
  {
  case 8:
    {
      ga_int8* mix = (ga_int8*)out_buffer;
      for(i = 0; i < end; ++i)
      {
        ga_int32 sample = m->mixBuffer[i];
        mix[i] = (ga_int8)(sample > -128 ? (sample < 127 ? sample : 127) : -128);
      }
      break;
    }
  case 16:
    {
      ga_int16* mix = (ga_int16*)out_buffer;
      for(i = 0; i < end; ++i)
      {
        ga_int32 sample = m->mixBuffer[i];
        mix[i] = (ga_int16)(sample > -32768 ? (sample < 32767 ? sample : 32767) : -32768);
      }
      break;
    }
  }
  return GA_SUCCESS;
}
ga_result ga_mixer_dispatch(ga_Mixer* in_mixer)
{
  ga_Mixer* m = in_mixer;
  ga_Link* link = m->dispatchList.next;
  while(link != &m->dispatchList)
  {
    ga_Link* oldLink = link;
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
        ga_mutex_lock(m->dispatchMutex);
        ga_list_unlink(&oldHandle->dispatchLink);
        ga_mutex_unlock(m->dispatchMutex);
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
  return GA_SUCCESS;
}
ga_result ga_mixer_destroy(ga_Mixer* in_mixer)
{
  /* NOTE: Mixer/handles must no longer be in use on any thread when destroy is called */
  ga_Mixer* m = in_mixer;
  ga_Link* link;
  link = m->dispatchList.next;
  while(link != &m->dispatchList)
  {
    ga_Handle* oldHandle = (ga_Handle*)link->data;
    link = link->next;
    gaX_handle_cleanup(oldHandle);
  }

  ga_mutex_destroy(in_mixer->dispatchMutex);
  ga_mutex_destroy(in_mixer->mixMutex);
  ga_mutex_destroy(in_mixer->streamMutex);

  gaX_cb->freeFunc(in_mixer->mixBuffer);
  gaX_cb->freeFunc(in_mixer);
  return GA_SUCCESS;
}

/* Circular Buffer Functions */
ga_CircBuffer* ga_buffer_create(ga_uint32 in_size)
{
  ga_CircBuffer* ret;
  if(!in_size || (in_size & (in_size - 1))) /* Must be power-of-two*/
    return 0;
  ret = gaX_cb->allocFunc(sizeof(ga_CircBuffer));
  ret->data = gaX_cb->allocFunc(in_size);
  ret->dataSize = in_size;
  ret->nextAvail = 0;
  ret->nextFree = 0;
  return ret;
}
ga_result ga_buffer_destroy(ga_CircBuffer* in_buffer)
{
  gaX_cb->freeFunc(in_buffer);
  return GA_SUCCESS;
}
ga_uint32 ga_buffer_bytesAvail(ga_CircBuffer* in_buffer)
{
  /* producer/consumer call (all race-induced errors should be tolerable) */
  return in_buffer->nextFree - in_buffer->nextAvail;
}
ga_uint32 ga_buffer_bytesFree(ga_CircBuffer* in_buffer)
{
  /* producer/consumer call (all race-induced errors should be tolerable) */
  return in_buffer->dataSize - ga_buffer_bytesAvail(in_buffer);
}
ga_int32 ga_buffer_getFree(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes,
                           void** out_dataA, ga_uint32* out_sizeA,
                           void** out_dataB, ga_uint32* out_sizeB)
{
  /* producer-only call */
  ga_CircBuffer* b = in_buffer;
  ga_uint32 size = b->dataSize;
  ga_uint32 nextFree = b->nextFree % size;
  ga_uint32 nextAvail = b->nextAvail % size;
  ga_uint32 maxBytes = size - nextFree;
  if(in_numBytes > ga_buffer_bytesFree(b))
    return -1;
  if(maxBytes >= in_numBytes)
  {
    *out_dataA = &b->data[nextFree];
    *out_sizeA = in_numBytes;
    return 1;
  }
  else
  {
    *out_dataA = &b->data[nextFree];
    *out_sizeA = maxBytes;
    *out_dataB = &b->data[0];
    *out_sizeB = in_numBytes - maxBytes;
    return 2;
  }
}
ga_result ga_buffer_write(ga_CircBuffer* in_buffer, void* in_data,
                          ga_uint32 in_numBytes)
{
  /* TODO: Make this call ga_buffer_getFree() instead of duping code */

  /* producer-only call */
  ga_CircBuffer* b = in_buffer;
  ga_uint32 size = b->dataSize;
  ga_uint32 nextFree = b->nextFree % size;
  ga_uint32 nextAvail = b->nextAvail % size;
  ga_uint32 maxBytes = size - nextFree;
  if(in_numBytes > ga_buffer_bytesFree(b))
    return GA_ERROR_GENERIC;
  if(maxBytes >= in_numBytes)
    memcpy(&b->data[nextFree], in_data, in_numBytes);
  else
  {
    memcpy(&b->data[nextFree], in_data, maxBytes);
    memcpy(&b->data[0], (char*)in_data + maxBytes, in_numBytes - maxBytes);
  }
  b->nextFree += in_numBytes;
  return GA_SUCCESS;
}
ga_int32 ga_buffer_getAvail(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes,
                            void** out_dataA, ga_uint32* out_sizeA,
                            void** out_dataB, ga_uint32* out_sizeB)
{
  /* consumer-only call */
  ga_CircBuffer* b = in_buffer;
  ga_uint32 bytesAvailable = ga_buffer_bytesAvail(in_buffer);
  ga_uint32 size = b->dataSize;
  ga_uint32 nextFree = b->nextFree % size;
  ga_uint32 nextAvail = b->nextAvail % size;
  ga_uint32 maxBytes = size - nextAvail;
  if(bytesAvailable < in_numBytes)
    return -1;
  if(maxBytes >= in_numBytes)
  {
    *out_dataA = &b->data[nextAvail];
    *out_sizeA = in_numBytes;
    return 1;
  }
  else
  {
    *out_dataA = &b->data[nextAvail];
    *out_sizeA = maxBytes;
    *out_dataB = &b->data[0];
    *out_sizeB = in_numBytes - maxBytes;
    return 2;
  }
}
void ga_buffer_read(ga_CircBuffer* in_buffer, void* in_data,
                    ga_uint32 in_numBytes)
{
  /* consumer-only call */
  void* data[2];
  ga_uint32 size[2];
  ga_int32 ret = ga_buffer_getAvail(in_buffer, in_numBytes,
                                   &data[0], &size[0],
                                   &data[1], &size[1]);
  if(ret >= 1)
  {
    memcpy(in_data, data[0], size[0]);
    if(ret == 2)
      memcpy((char*)in_data + size[0], data[1], size[1]);
  }
}
void ga_buffer_consume(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes)
{
  /* consumer-only call */
  in_buffer->nextAvail += in_numBytes;
}
