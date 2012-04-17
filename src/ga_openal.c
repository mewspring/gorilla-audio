#include "gorilla/ga.h"

#include "gorilla/ga_openal.h"

#include "al.h"
#include "alc.h"

#include <stdlib.h>

ga_DeviceImpl_OpenAl* gaX_device_open_openAl()
{
  ga_DeviceImpl_OpenAl* ret = malloc(sizeof(ga_DeviceImpl_OpenAl));
  ALCboolean ctxRet;

  ret->devType = GA_DEVICE_TYPE_OPENAL;
#ifdef _WIN32
  ret->dev = alcOpenDevice("DirectSound");
#else
  ret->dev = alcOpenDevice(0);
#endif //WIN32
  if(!ret->dev)
    goto cleanup;
  ret->context = alcCreateContext(ret->dev, 0);
  if(!ret->context)
    goto cleanup;
  ctxRet = alcMakeContextCurrent(ret->context);
  if(ctxRet == ALC_FALSE)
    goto cleanup;
  return ret;

cleanup:
  if(ret->context)
    alcDestroyContext(ret->context);
  if(ret->dev)
    alcCloseDevice(ret->dev);
  free(ret);
  return 0;
}
void gaX_device_close_openAl(ga_DeviceImpl_OpenAl* in_dev)
{
  alcCloseDevice(in_dev->dev);
  in_dev->devType = GA_DEVICE_TYPE_UNKNOWN;
  free(in_dev);
}
