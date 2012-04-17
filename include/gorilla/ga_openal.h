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
} ga_DeviceImpl_OpenAl;

ga_DeviceImpl_OpenAl* gaX_device_open_openAl();
void gaX_device_close_openAl(ga_DeviceImpl_OpenAl* in_dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LINK_AGAINST_OPENAL */

#endif /* _GORILLA_GA_OPENAL_H */
