#ifdef LINK_AGAINST_OPENAL

#include "gorilla/ga.h"

#include "gorilla/ga_openal.h"

#include "al.h"
#include "alc.h"

#include <stdlib.h>
#include <stdio.h>

const char* gaX_openAlErrorToString(ALuint error)
{
  const char* errMsg = 0;
  switch(error)
  {
  case AL_NO_ERROR: errMsg = "OpenAL error - None"; break;
  case AL_INVALID_NAME: errMsg = "OpenAL error - Invalid name."; break;
  case AL_INVALID_ENUM: errMsg = "OpenAL error - Invalid enum."; break;
  case AL_INVALID_VALUE: errMsg = "OpenAL error - Invalid value."; break;
  case AL_INVALID_OPERATION: errMsg = "OpenAL error - Invalid op."; break;
  case AL_OUT_OF_MEMORY: errMsg = "OpenAL error - Out of memory."; break;
  default: errMsg = "OpenAL error - Unknown error."; break;
  }
  return errMsg;
}

static ga_int32 AUDIO_ERROR = 0;

#define CHECK_AL_ERROR \
  if((AUDIO_ERROR = alGetError()) != AL_NO_ERROR) \
    printf("%s\n", gaX_openAlErrorToString(AUDIO_ERROR));

ga_DeviceImpl_OpenAl* gaX_device_open_openAl()
{
  ga_DeviceImpl_OpenAl* ret = gaX_cb->allocFunc(sizeof(ga_DeviceImpl_OpenAl));
  ALCboolean ctxRet;

  ret->devType = GA_DEVICE_TYPE_OPENAL;
  ret->nextBuffer = 0;
  ret->emptyBuffers = 2;
#ifdef _WIN32
  ret->dev = alcOpenDevice("DirectSound");
#else
  ret->dev = alcOpenDevice(0);
#endif /* _WIN32 */
  if(!ret->dev)
    goto cleanup;
  ret->context = alcCreateContext(ret->dev, 0);
  if(!ret->context)
    goto cleanup;
  ctxRet = alcMakeContextCurrent(ret->context);
  if(ctxRet == ALC_FALSE)
    goto cleanup;

  alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
  CHECK_AL_ERROR;
  if(AUDIO_ERROR != AL_NO_ERROR)
    goto cleanup;
  alGenBuffers(2, &ret->hwBuffers[0]);
  CHECK_AL_ERROR;
  if(AUDIO_ERROR != AL_NO_ERROR)
    goto cleanup;
  alGenSources(1, &ret->hwSource);
  CHECK_AL_ERROR;
  if(AUDIO_ERROR != AL_NO_ERROR)
  {
    alDeleteBuffers(2, &ret->hwBuffers[0]);
    goto cleanup;
  }
  return ret;

cleanup:
  if(ret->context)
    alcDestroyContext(ret->context);
  if(ret->dev)
    alcCloseDevice(ret->dev);
  gaX_cb->freeFunc(ret);
  return 0;
}
ga_result gaX_device_close_openAl(ga_DeviceImpl_OpenAl* in_dev)
{
  alcCloseDevice(in_dev->dev);
  in_dev->devType = GA_DEVICE_TYPE_UNKNOWN;
  gaX_cb->freeFunc(in_dev);
  return GA_SUCCESS;
}
ga_int32 gaX_device_check_openAl(ga_DeviceImpl_OpenAl* in_device)
{
  ga_int32 whichBuf = 0;
  ga_DeviceImpl_OpenAl* d = in_device;
  ga_int32 numProcessed = 0;
  alGetSourcei(in_device->hwSource, AL_BUFFERS_PROCESSED, &numProcessed);
  CHECK_AL_ERROR;
  while(numProcessed--)
  {
    whichBuf = (d->nextBuffer + d->emptyBuffers++) % 2;
    alSourceUnqueueBuffers(d->hwSource, 1, &d->hwBuffers[whichBuf]);
    CHECK_AL_ERROR;
  }
  return d->emptyBuffers;
}
ga_result gaX_device_queue_openAl(ga_DeviceImpl_OpenAl* in_device,
                                 ga_Format* in_format,
                                 ga_int32 in_numSamples,
                                 void* in_buffer)
{
  ga_int32 formatOal;
  ga_int32 sampleSize;
  ALint state;
  ga_int32 bps = in_format->bitsPerSample;
  ga_DeviceImpl_OpenAl* d = in_device;

  if(in_format->numChannels == 1)
    formatOal = (ga_int32)(bps == 16 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8);
  else
    formatOal = (ga_int32)(bps == 16 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8);
  sampleSize = ga_format_sampleSize(in_format);
  alBufferData(d->hwBuffers[d->nextBuffer], formatOal, in_buffer,
    (ALsizei)in_numSamples * sampleSize, in_format->sampleRate);
  CHECK_AL_ERROR;
  if(AUDIO_ERROR != AL_NO_ERROR)
    return GA_ERROR_GENERIC;
  alSourceQueueBuffers(d->hwSource, 1, &d->hwBuffers[d->nextBuffer]);
  CHECK_AL_ERROR;
  if(AUDIO_ERROR != AL_NO_ERROR)
    return GA_ERROR_GENERIC;
  d->nextBuffer = (d->nextBuffer + 1) % 2;
  --d->emptyBuffers;
  alGetSourcei(d->hwSource, AL_SOURCE_STATE, &state);
  CHECK_AL_ERROR;
  if(state != AL_PLAYING)
  {
    /* NOTE: calling this, even as a 'noop', can cause a clicking sound. */
    alSourcePlay(d->hwSource);
  }
  CHECK_AL_ERROR;
  return GA_SUCCESS;
}

#endif /* LINK_AGAINST_OPENAL */
