#include "gorilla/ga.h"

#include "gorilla/ga_openal.h"

/* System Functions */
ga_SystemCallbacks* gaX_cb = 0;
ga_result ga_initialize(ga_SystemCallbacks* in_callbacks)
{
  gaX_cb = in_callbacks;
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