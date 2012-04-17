#include "gorilla/ga.h"

#include "gorilla/ga_openal.h"

void ga_initialize(ga_SystemCallbacks* in_callbacks)
{
}
void ga_shutdown()
{
}

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
ga_int32 ga_device_close(ga_Device* in_device)
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
