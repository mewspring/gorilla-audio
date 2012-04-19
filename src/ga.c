#include "gorilla/ga.h"

#include "gorilla/ga_wav.h"
#include "gorilla/ga_openal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* System Functions */
ga_SystemOps* gaX_cb = 0;

static void* gaX_defaultAllocFunc(ga_uint32 in_size)
{
  return malloc(in_size);
}
static void gaX_defaultFreeFunc(void* in_ptr)
{
  free(in_ptr);
}
static ga_SystemOps s_defaultCallbacks;
ga_result ga_initialize(ga_SystemOps* in_callbacks)
{
  s_defaultCallbacks.allocFunc = &gaX_defaultAllocFunc;
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
ga_result gaX_sound_load_wav(ga_Sound* in_ret, const char* in_filename, ga_uint32 in_byteOffset);
ga_result gaX_sound_load_ogg(ga_Sound* in_ret, const char* in_filename, ga_uint32 in_byteOffset);
ga_Sound* ga_sound_load(const char* in_filename, ga_int32 in_fileFormat,
                        ga_uint32 in_byteOffset)
{
  ga_Sound* ret = gaX_cb->allocFunc(sizeof(ga_Sound));
  ret->data = 0;
  ret->isCopy = 1;
  switch(in_fileFormat)
  {
  case GA_FILE_FORMAT_WAV:
    if(gaX_sound_load_wav(ret, in_filename, in_byteOffset) != GA_SUCCESS)
    {
      gaX_cb->freeFunc(ret);
      return 0;
    }
    ret->loopStart = 0;
    ret->loopEnd = ga_sound_numSamples((ga_Sound*)ret);
    return (ga_Sound*)ret;

  case GA_FILE_FORMAT_OGG:
    if(gaX_sound_load_ogg(ret, in_filename, in_byteOffset) != GA_SUCCESS)
    {
      gaX_cb->freeFunc(ret);
      return 0;
    }
    return ret;
  }
  return 0;
}
ga_Sound* ga_sound_assign(void* in_buffer, ga_int32 in_size,
                          ga_Format* in_format, ga_int32 in_copy)
{
  ga_Sound* ret = gaX_cb->allocFunc(sizeof(ga_Sound));
  ret->isCopy = in_copy;
  ret->size = in_size;
  if(in_copy)
  {
    ret->data = gaX_cb->allocFunc(in_size);
    memcpy(ret->data, in_buffer, in_size);
  }
  else
    ret->data = in_buffer;
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

/* Sound File-Loading Functions */
ga_result gaX_sound_load_wav(ga_Sound* in_ret, const char* in_filename, ga_uint32 in_byteOffset)
{
  FILE* f;
  ga_WavData wavData;
  ga_int32 validHdr = gaX_sound_load_wav_header(in_filename, in_byteOffset, &wavData);
  if(validHdr != GA_SUCCESS)
    return GA_ERROR_GENERIC;
  f = fopen(in_filename, "rb");
  if(!f)
    return GA_ERROR_GENERIC;
  in_ret->size = wavData.dataSize;
  in_ret->format.bitsPerSample = wavData.bitsPerSample;
  in_ret->format.numChannels = wavData.channels;
  in_ret->format.sampleRate = wavData.sampleRate;
  in_ret->data = gaX_cb->allocFunc(in_ret->size);
  fseek(f, wavData.dataOffset, SEEK_SET);
  fread(in_ret->data, 1, wavData.dataSize, f);
  return GA_SUCCESS;
}
ga_result gaX_sound_load_ogg(ga_Sound* in_ret, const char* in_filename, ga_uint32 in_byteOffset)
{
  /* TODO! */
  return GA_ERROR_GENERIC;
}

/* Handle Functions */
void gaX_mixer_addHandle(ga_Mixer* in_mixer, ga_Handle* in_handle);
void gaX_mixer_removeHandle(ga_Mixer* in_mixer, ga_Handle* in_handle);

void gaX_handle_init(ga_Handle* in_handle, ga_Mixer* in_mixer)
{
  ga_Handle* h = in_handle;
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
}

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_Sound* in_sound)
{
  ga_HandleStatic* hs = (ga_HandleStatic*)gaX_cb->allocFunc(sizeof(ga_HandleStatic));
  ga_Handle* h = (ga_Handle*)hs;
  h->handleType = GA_HANDLE_TYPE_STATIC;
  hs->nextSample = 0;
  hs->sound = in_sound;
  gaX_handle_init(h, in_mixer);
  gaX_mixer_addHandle(in_mixer, h);
  return h;
}
ga_Handle* ga_handle_createStream(ga_Mixer* in_mixer,
                                  ga_StreamProduceFunc in_createFunc,
                                  ga_StreamDestroyFunc in_destroyFunc,
                                  void* in_context,
                                  ga_int32 in_group)
{
  ga_HandleStream* hs = (ga_HandleStream*)gaX_cb->allocFunc(sizeof(ga_HandleStream));
  ga_Handle* h = (ga_Handle*)hs;
  h->handleType = GA_HANDLE_TYPE_STREAM;
  hs->createFunc = in_createFunc;
  hs->destroyFunc = in_destroyFunc;
  hs->streamContext = in_context;
  hs->group = in_group;
  gaX_handle_init(h, in_mixer);
  gaX_mixer_addHandle(in_mixer, h);
  return h;
}
ga_result ga_handle_destroy(ga_Handle* in_handle)
{
  ga_Mixer* m = in_handle->mixer;
  gaX_mixer_removeHandle(m, in_handle);
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* hs = (ga_HandleStatic*)in_handle;
      gaX_cb->freeFunc(hs);
      break;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      ga_HandleStream* hs = (ga_HandleStream*)in_handle;
      hs->destroyFunc(hs->context);
      gaX_cb->freeFunc(hs);
      break;
    }
  }
  return GA_SUCCESS;
}

#define GA_HANDLE_STATE_INITIAL 0
#define GA_HANDLE_STATE_PLAYING 1
#define GA_HANDLE_STATE_STOPPED 2
#define GA_HANDLE_STATE_FINISHED 3

ga_result ga_handle_play(ga_Handle* in_handle)
{
  if(in_handle->state == GA_HANDLE_STATE_FINISHED)
    return GA_ERROR_GENERIC;
  in_handle->state = GA_HANDLE_STATE_PLAYING;
  return GA_SUCCESS;
}
ga_result ga_handle_stop(ga_Handle* in_handle)
{
  if(in_handle->state == GA_HANDLE_STATE_FINISHED)
    return GA_ERROR_GENERIC;
  in_handle->state = GA_HANDLE_STATE_STOPPED;
  return GA_SUCCESS;
}
ga_int32 ga_handle_finished(ga_Handle* in_handle)
{
  return in_handle->state == GA_HANDLE_STATE_FINISHED ? GA_TRUE : GA_FALSE;
}
ga_result ga_handle_setCallback(ga_Handle* in_handle, ga_FinishCallback in_callback, void* in_context)
{
  in_handle->callback = in_callback;
  in_handle->context = in_context;
  return GA_SUCCESS;
}
ga_result ga_handle_setParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: h->envDuration = -1; h->gain = in_value; break;
  case GA_HANDLE_PARAM_PAN: h->pan = in_value; break;
  case GA_HANDLE_PARAM_PITCH: h->pitch = in_value; break;
  case GA_HANDLE_PARAM_LOOP: return GA_ERROR_GENERIC;
  }
  return GA_SUCCESS;
}
ga_result ga_handle_getParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: *out_value = h->gain; break;
  case GA_HANDLE_PARAM_PAN: *out_value = h->pan; break;
  case GA_HANDLE_PARAM_PITCH: *out_value = h->pitch; break;
  case GA_HANDLE_PARAM_LOOP: return GA_ERROR_GENERIC;
  }
  return GA_SUCCESS;
}
ga_result ga_handle_setParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32 in_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_PAN: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_PITCH: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_LOOP: h->loop = in_value; break;
  }
  return GA_SUCCESS;
}
ga_result ga_handle_getParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32* out_value)
{
  ga_Handle* h = in_handle;
  switch(in_param)
  {
  case GA_HANDLE_PARAM_GAIN: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_PAN: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_PITCH: return GA_ERROR_GENERIC;
  case GA_HANDLE_PARAM_LOOP: *out_value = h->loop; break;
  }
  return GA_SUCCESS;
}
ga_result ga_handle_seek(ga_Handle* in_handle, ga_int32 in_sampleOffset)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* hs = (ga_HandleStatic*)gaX_cb->allocFunc(sizeof(ga_HandleStatic));
      if(in_sampleOffset >= ga_sound_numSamples(in_handle->sound))
        in_sampleOffset = 0;
      hs->nextSample = in_sampleOffset;
      break;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      /* TODO! */
      break;
    }
  }
  return GA_SUCCESS;
}
ga_int32 ga_handle_tell(ga_Handle* in_handle)
{
  switch(in_handle->handleType)
  {
  case GA_HANDLE_TYPE_STATIC:
    {
      ga_HandleStatic* hs = (ga_HandleStatic*)gaX_cb->allocFunc(sizeof(ga_HandleStatic));
      return hs->nextSample;
    }
  case GA_HANDLE_TYPE_STREAM:
    {
      /* TODO! */
      return 0;
    }
  }
  return 0;
}

ga_result ga_handle_envelope(ga_Handle* in_handle, ga_int32 in_duration,
                             ga_float32 in_gain)
{
  in_handle->envDuration = in_duration;
  in_handle->envGainA = in_handle->gain;
  in_handle->envGainB = in_gain;
  return GA_SUCCESS;
}

ga_result ga_handle_sync(ga_Handle* in_handle, ga_int32 in_group);
ga_result ga_handle_desync(ga_Handle* in_handle);

ga_result ga_handle_lock(ga_Handle* in_handle);
ga_result ga_handle_unlock(ga_Handle* in_handle);

/* Mixer Functions */
ga_Mixer* ga_mixer_create(ga_Format* in_format, ga_int32 in_numSamples)
{
  ga_Mixer* ret = gaX_cb->allocFunc(sizeof(ga_Mixer));
  ga_int32 mixSampleSize;
  ret->handles.next = &ret->handles;
  ret->handles.prev = &ret->handles;
  ret->numSamples = in_numSamples;
  memcpy(&ret->format, in_format, sizeof(ga_Format));
  ret->mixFormat.bitsPerSample = 32;
  ret->mixFormat.numChannels = in_format->numChannels;
  ret->mixFormat.sampleRate = in_format->sampleRate;
  mixSampleSize = ga_format_sampleSize(&ret->mixFormat);
  ret->mixBuffer = (ga_int32*)gaX_cb->allocFunc(in_numSamples * mixSampleSize);
  return ret;
}
void gaX_mixer_mix_static(ga_Mixer* in_mixer, ga_HandleStatic* in_handle, ga_int32* in_buffer, ga_int32 in_numSamples)
{
  /* mutex.lock() */
  if(in_handle->state == GA_HANDLE_STATE_PLAYING)
  {
    ga_HandleStatic* h = in_handle;
    ga_Sound* s = h->sound;
    ga_Format* mixFmt = &in_mixer->format;
    ga_int32 mixerChannels = mixFmt->numChannels;
    ga_int32 soundChannels = s->format.numChannels;
    ga_int32 numSamples = (ga_int32)ga_sound_numSamples((ga_Sound*)s);
    ga_float32 sampleScale = s->format.sampleRate / (ga_float32)mixFmt->sampleRate * h->pitch;
    ga_int32* dst = in_buffer;
    ga_int32 endSample = h->loop ? s->loopEnd : numSamples;
    ga_int32 numToFill = in_numSamples;
    ga_float32 fj = 0.0f;
    ga_int32 j = 0;
    ga_int32 i = 0;
    ga_int32 available;
    ga_float32 pan;

    /* TODO: is this a robust-enough check if looping is turned on after passing loopEnd? */
    available = endSample - h->nextSample;
    available = available > 0 ? available : 0;

    pan = (h->pan + 1.0f) / 2.0f;
    pan = pan > 1.0f ? 1.0f : pan;
    pan = pan < 0.0f ? 0.0f : pan;

    /* TODO: Support stereo mixer format */
    switch(s->format.bitsPerSample)
    {
    case 8:
      {
        const ga_uint8* src = (const ga_uint8*)s->data;
        src += h->nextSample * soundChannels;
        while(i < numToFill * mixerChannels && j < available * soundChannels)
        {
          dst[i] += (ga_int32)(((ga_int32)src[j] - 128) * h->gain * (1.0f - pan) * 256.0f * 2);
          dst[i + 1] += (ga_int32)(((ga_int32)src[j + ((soundChannels == 1) ? 0 : 1)] - 128) * h->gain * pan * 256.0f * 2);
          fj += sampleScale * soundChannels;
          j = (ga_uint32)fj;
          j = j & ((soundChannels == 1) ? ~0 : ~0x1);
          j = j > available * soundChannels ? available * soundChannels : j;
          i += mixerChannels;
        }
        break;
      }
    case 16:
      {
        const ga_int16* src = (const ga_int16*)s->data;
        src += h->nextSample * soundChannels;
        while(i < numToFill * (ga_int32)mixerChannels && j < available * soundChannels)
        {
          dst[i] += (ga_int32)((ga_int32)src[j] * h->gain * (1.0f - pan) * 2);
          dst[i + 1] += (ga_int32)((ga_int32)src[j + ((soundChannels == 1) ? 0 : 1)] * h->gain * pan * 2);
          fj += sampleScale * soundChannels;
          j = (ga_uint32)fj;
          j = j & (soundChannels == 1 ? ~0 : ~0x1);
          j = j > available * soundChannels ? available * soundChannels : j;
          i += mixerChannels;
        }
        break;
      }
    }
    j = (ga_uint32)fj;
    j = j & ((soundChannels == 1) ? ~0 : ~0x1);
    j = j > available * soundChannels ? available * soundChannels : j;
    h->nextSample += j / soundChannels;
    if(h->nextSample >= endSample)
    {
      if(h->loop)
      {
        ga_int32 samplesMixed = i / mixerChannels;
        ga_int32 samplesRemaining = numToFill - samplesMixed;
        h->nextSample = s->loopStart;
        if(samplesRemaining > 0)
        {
          /* mutex.unlock() */
          gaX_mixer_mix_static(in_mixer, in_handle,
            dst + samplesMixed * mixerChannels, samplesRemaining);
          return;
        }
      }
      else
      {
        h->state = GA_HANDLE_STATE_FINISHED;
      }
    }
  }
  /* mutex.lock() */
}
void gaX_mixer_mix_streaming(ga_Mixer* in_mixer, ga_HandleStream* in_handle)
{
  /* TODO! */
}
ga_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer)
{
  ga_int32 i;
  ga_Mixer* m = in_mixer;
  ga_int32 end = m->numSamples * m->format.numChannels;
  ga_Format* fmt = &m->format;
  ga_Handle* h = m->handles.next;
  ga_int32 mixSampleSize = ga_format_sampleSize(&m->mixFormat);
  memset(&m->mixBuffer[0], 0, m->numSamples * mixSampleSize);
  while(h != &m->handles)
  {
    switch(h->handleType)
    {
    case GA_HANDLE_TYPE_STATIC:
      {
        ga_HandleStatic* hs = (ga_HandleStatic*)h;
        gaX_mixer_mix_static(m, hs, &m->mixBuffer[0], m->numSamples);
        break;
      }
    case GA_HANDLE_TYPE_STREAM:
      {
        ga_HandleStream* hs = (ga_HandleStream*)h;
        gaX_mixer_mix_streaming(m, hs);
        break;
      }
    }
    h = h->next;
  }

  /* Remove finished handles */
  h = m->handles.next;
  while(h != &m->handles)
  {
    ga_Handle* oldHandle = h;
    h = h->next;
    if(ga_handle_finished(oldHandle))
    {
      gaX_mixer_removeHandle(oldHandle->mixer, oldHandle);
      if(oldHandle->callback)
        oldHandle->callback(oldHandle, oldHandle->context);
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
ga_result ga_mixer_destroy(ga_Mixer* in_mixer)
{
  gaX_cb->freeFunc(in_mixer->mixBuffer);
  gaX_cb->freeFunc(in_mixer);
  return GA_SUCCESS;
}
void gaX_mixer_addHandle(ga_Mixer* in_mixer, ga_Handle* in_handle)
{
  in_handle->prev = &in_mixer->handles;
  in_handle->next = in_mixer->handles.next;
  in_mixer->handles.next->prev = in_handle;
  in_mixer->handles.next = in_handle;
}
void gaX_mixer_removeHandle(ga_Mixer* in_mixer, ga_Handle* in_handle)
{
  in_handle->prev->next = in_handle->next;
  in_handle->next->prev = in_handle->prev;
  in_handle->prev = in_handle;
  in_handle->next = in_handle;
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
ga_result ga_buffer_write(ga_CircBuffer* in_buffer, void* in_data,
                          ga_uint32 in_numBytes)
{
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
ga_int32 ga_buffer_getData(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes,
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
  ga_int32 ret = ga_buffer_getData(in_buffer, in_numBytes,
                                   &data[0], &size[0],
                                   &data[1], &size[1]);
  if(ret == 1)
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
