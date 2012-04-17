#ifndef _GORILLA_GA_H
#define _GORILLA_GA_H

#include "ga_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Error Codes
  */
#define GA_FALSE 0
#define GA_TRUE 1
#define GA_SUCCESS 1
#define GA_ERROR_GENERIC -1

typedef struct ga_SystemCallbacks{
  ga_int32 _dummy;
  // alloc
  // free
  // error
} ga_SystemCallbacks;

void ga_initialize(ga_SystemCallbacks* in_callbacks);
void ga_shutdown();

/*
  Gorilla Audio Device
*/
#define GA_DEVICE_TYPE_ANY -1
#define GA_DEVICE_TYPE_UNKNOWN 0
#define GA_DEVICE_TYPE_OPENAL 1

#define GA_DEVICE_HEADER \
  ga_int32 devType;

typedef struct ga_Device {
  GA_DEVICE_HEADER
} ga_Device;

ga_Device* ga_device_open(ga_int32 in_type);
ga_int32 ga_device_check();
ga_int32 ga_device_queue();
ga_int32 ga_device_close(ga_Device* in_device);

/*
 Gorilla Software Mixer
*/
typedef struct ga_Mixer {
  ga_int32 _dummy;
} ga_Mixer;

ga_Mixer* ga_mixer_create(ga_int32 in_sampleRate, ga_int32 in_bitsPerSample, ga_int32 in_numChannels);
ga_int32 ga_mixer_latency(ga_Mixer* in_mixer);
ga_int32 ga_mixer_sampleSize(ga_Mixer* in_mixer);
void ga_mixer_mix(ga_Mixer* in_mixer, ga_int32 in_numSamples, char* out_buffer);
void ga_mixer_destroy(ga_Mixer* in_mixer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
