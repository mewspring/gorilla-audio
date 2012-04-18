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
typedef ga_int32 ga_result;

#define GA_FALSE 0
#define GA_TRUE 1
#define GA_SUCCESS 1
#define GA_ERROR_GENERIC -1

/*
  Gorilla System
*/
typedef struct ga_SystemCallbacks {
  void* (*allocFunc)(ga_uint32 in_size);
  void (*freeFunc)(void* in_ptr);
} ga_SystemCallbacks;
extern ga_SystemCallbacks* gaX_cb;

ga_result ga_initialize(ga_SystemCallbacks* in_callbacks);
ga_result ga_shutdown();

/*
  Gorilla Audio Format
*/
typedef struct ga_Format {
  ga_int32 sampleRate;
  ga_int32 bitsPerSample;
  ga_int32 numChannels;
} ga_Format;

ga_int32 ga_format_sampleSize(ga_Format* in_format);

/*
  Gorilla Audio Device
*/
#define GA_DEVICE_TYPE_ANY -1
#define GA_DEVICE_TYPE_UNKNOWN 0
#define GA_DEVICE_TYPE_OPENAL 1

#define GA_DEVICE_HEADER ga_int32 devType;

typedef struct ga_Device {
  GA_DEVICE_HEADER
} ga_Device;

ga_Device* ga_device_open(ga_int32 in_type);
ga_int32 ga_device_check(ga_Device* in_device);
ga_result ga_device_queue(ga_Device* in_device,
                         ga_Format* in_format,
                         ga_int32 in_numSamples,
                         void* in_buffer);
ga_result ga_device_close(ga_Device* in_device);

/*
 Gorilla Streaming
*/
typedef struct ga_StreamManager {
  ga_int32 _unused;
} ga_StreamManager;

ga_StreamManager* ga_stream_manager_create();
ga_result ga_stream_manager_update(ga_StreamManager* in_manager);
ga_result ga_stream_manager_destroy(ga_StreamManager* in_manager);

/*
  Gorilla Sound
  */
#define GA_SOUND_TYPE_UNKNOWN 0
#define GA_SOUND_TYPE_STATIC 1
#define GA_SOUND_TYPE_STREAMING 2

#define GA_SOUND_HEADER ga_int32 soundType;

typedef struct ga_Sound {
  GA_SOUND_HEADER
} ga_Sound;

typedef struct ga_SoundStatic {
  GA_SOUND_HEADER
  void* data;
  ga_uint32 size;
  ga_Format format;
  ga_int32 loopStart; /* in samples */
  ga_int32 loopEnd; /* in samples*/
} ga_SoundStatic;

#define GA_STREAM_TYPE_UNKNOWN 0
#define GA_STREAM_TYPE_AUTO 1
#define GA_STREAM_TYPE_MANUAL 2
#define GA_STREAM_HEADER ga_int32 streamType;

typedef struct ga_SoundStreaming {
  GA_SOUND_HEADER
  GA_STREAM_HEADER
} ga_SoundStreaming;

#define GA_FILE_FORMAT_UNKNOWN 0
#define GA_FILE_FORMAT_WAV 1
#define GA_FILE_FORMAT_OGG 2

ga_Sound* ga_sound_load(const char* in_filename, ga_int32 in_fileFormat,
                        ga_uint32 in_byteOffset);
ga_Sound* ga_sound_stream(const char* in_filename,
                          ga_int32 in_fileFormat,
                          ga_uint32 in_byteOffset,
                          ga_StreamManager* in_manager);
ga_Sound* ga_sound_assign(void* in_buffer, ga_int32 in_size,
                          ga_Format* in_format, ga_int32 in_copy);
ga_result ga_sound_setLoops(ga_int32 in_loopStart, ga_int32 in_loopEnd);
ga_result ga_sound_destroy(ga_Sound* in_sound);

/*
  Gorilla Handle
*/
#define GA_HANDLE_PARAM_UNKNOWN 0
#define GA_HANDLE_PARAM_PAN 1
#define GA_HANDLE_PARAM_PITCH 2
#define GA_HANDLE_PARAM_GAIN 3
#define GA_HANDLE_PARAM_LOOP 4

typedef struct ga_Handle {
  ga_int32 _unused;
  /* intrusive stream group linked list */
  /*
  */
} ga_Handle;

ga_Handle* ga_handle_create(ga_Sound* in_sound);
ga_result ga_handle_destroy(ga_Handle* in_handle);

ga_result ga_handle_play(ga_Handle* in_handle);
ga_result ga_handle_stop(ga_Handle* in_handle);
ga_result ga_handle_rewind(ga_Handle* in_handle);

ga_result ga_handle_sync(ga_Handle* in_handle, ga_int32 in_group);
ga_result ga_handle_desync(ga_Handle* in_handle);

ga_result ga_handle_seek(ga_Handle* in_handle, ga_int32 in_sampleOffset);
ga_int32 ga_handle_tell(ga_Handle* in_handle);

ga_result ga_handle_setParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32 in_value);
ga_result ga_handle_getParamf(ga_Handle* in_handle, ga_int32 in_param,
                              ga_float32* out_value);

ga_result ga_handle_setParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32 in_value);
ga_result ga_handle_getParami(ga_Handle* in_handle, ga_int32 in_param,
                              ga_int32* out_value);

ga_result ga_handle_envelope(ga_Handle* in_handle, ga_int32 in_duration,
                             ga_float32 in_gain);

ga_result ga_handle_lock(ga_Handle* in_handle);
ga_result ga_handle_unlock(ga_Handle* in_handle);

/*
 Gorilla Mixer
*/
typedef struct ga_Mixer {
  ga_Format format;
  ga_int32 latency; /* in samples */
} ga_Mixer;

ga_Mixer* ga_mixer_create(ga_Format* in_format);
ga_result ga_mixer_mix(ga_Mixer* in_mixer,
                       ga_int32 in_numSamples,
                       void* out_buffer);
ga_result ga_mixer_destroy(ga_Mixer* in_mixer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
