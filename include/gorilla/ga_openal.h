#ifndef _GORILLA_GA_OPENAL_H
#define _GORILLA_GA_OPENAL_H

#ifdef LINK_AGAINST_OPENAL

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

typedef struct ga_DeviceImpl_OpenAl
{
  GA_DEVICE_HEADER
  ALCdevice* dev;
  ALCcontext* context;
  gc_uint32* hwBuffers;
  gc_uint32 hwSource;
  gc_uint32 nextBuffer;
  gc_uint32 emptyBuffers;
  gc_int32 numBuffers;
} ga_DeviceImpl_OpenAl;

ga_DeviceImpl_OpenAl* gaX_device_open_openAl(gc_int32 in_numBuffers);
gc_int32 gaX_device_check_openAl(ga_DeviceImpl_OpenAl* in_device);
gc_result gaX_device_queue_openAl(ga_DeviceImpl_OpenAl* in_device,
                                 ga_Format* in_format,
                                 gc_int32 in_numSamples,
                                 void* in_buffer);
gc_result gaX_device_close_openAl(ga_DeviceImpl_OpenAl* in_dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_AGAINST_OPENAL */

#endif /* _GORILLA_GA_OPENAL_H */
