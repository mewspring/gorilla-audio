#ifndef _GORILLA_GA_H
#define _GORILLA_GA_H

#include "ga_types.h"
#include "ga_thread.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Forward Declarations
*/
typedef struct ga_Handle ga_Handle;
typedef struct ga_Mixer ga_Mixer;

/*
  Circular Buffer
*/
typedef struct ga_CircBuffer {
  ga_uint8* data;
  ga_uint32 dataSize;
  ga_uint32 nextAvail;
  ga_uint32 nextFree;
} ga_CircBuffer;

ga_CircBuffer* ga_buffer_create(ga_uint32 in_size);
ga_result ga_buffer_destroy(ga_CircBuffer* in_buffer);
ga_uint32 ga_buffer_bytesAvail(ga_CircBuffer* in_buffer);
ga_uint32 ga_buffer_bytesFree(ga_CircBuffer* in_buffer);
ga_result ga_buffer_getFree(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes,
                            void** out_dataA, ga_uint32* out_sizeA,
                            void** out_dataB, ga_uint32* out_sizeB);
ga_result ga_buffer_write(ga_CircBuffer* in_buffer, void* in_data,
                          ga_uint32 in_numBytes);
ga_result ga_buffer_getAvail(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes,
                             void** out_dataA, ga_uint32* out_sizeA,
                             void** out_dataB, ga_uint32* out_sizeB);
void ga_buffer_read(ga_CircBuffer* in_buffer, void* in_data,
                    ga_uint32 in_numBytes);
void ga_buffer_consume(ga_CircBuffer* in_buffer, ga_uint32 in_numBytes);

/*
  Linked List
*/
typedef struct ga_Link ga_Link;
typedef struct ga_Link {
  ga_Link* next;
  ga_Link* prev;
  void* data;
} ga_Link;

void ga_list_head(ga_Link* in_head);
void ga_list_link(ga_Link* in_head, ga_Link* in_link, void* in_data);
void ga_list_unlink(ga_Link* in_link);

/*
  Gorilla System
*/
typedef struct ga_SystemOps {
  void* (*allocFunc)(ga_uint32 in_size);
  void* (*reallocFunc)(void* in_ptr, ga_uint32 in_size);
  void (*freeFunc)(void* in_ptr);
} ga_SystemOps;
extern ga_SystemOps* gaX_cb;

ga_result ga_initialize(ga_SystemOps* in_callbacks);
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
ga_float32 ga_format_toSeconds(ga_Format* in_format, ga_int32 in_samples);
ga_int32 ga_format_toSamples(ga_Format* in_format, ga_float32 in_seconds);

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

ga_Device* ga_device_open(ga_int32 in_type, ga_int32 in_numBuffers);
ga_int32 ga_device_check(ga_Device* in_device);
ga_result ga_device_queue(ga_Device* in_device,
                         ga_Format* in_format,
                         ga_int32 in_numSamples,
                         void* in_buffer);
ga_result ga_device_close(ga_Device* in_device);

/*
  Gorilla Sound
*/
typedef struct ga_Sound {
  ga_int32 loopStart;
  ga_int32 loopEnd;
  void* data;
  ga_uint32 size;
  ga_Format format;
  ga_int32 isCopy;
  ga_int32 numSamples;
} ga_Sound;

ga_Sound* ga_sound_create(void* in_data, ga_int32 in_size,
                          ga_Format* in_format, ga_int32 in_copy);
ga_result ga_sound_setLoops(ga_Sound* in_sound,
                            ga_int32 in_loopStart, ga_int32 in_loopEnd);
ga_int32 ga_sound_numSamples(ga_Sound* in_sound);
ga_result ga_sound_destroy(ga_Sound* in_sound);

/*
  Gorilla Handle
*/
#define GA_HANDLE_PARAM_UNKNOWN 0
#define GA_HANDLE_PARAM_PAN 1
#define GA_HANDLE_PARAM_PITCH 2
#define GA_HANDLE_PARAM_GAIN 3
#define GA_HANDLE_PARAM_LOOP 4

#define GA_HANDLE_STATE_UNKNOWN 0
#define GA_HANDLE_STATE_INITIAL 1
#define GA_HANDLE_STATE_PLAYING 2
#define GA_HANDLE_STATE_STOPPED 3
#define GA_HANDLE_STATE_FINISHED 4
#define GA_HANDLE_STATE_DESTROYED 5

typedef void (*ga_FinishCallback)(ga_Handle*, void*);

#define GA_HANDLE_HEADER \
  ga_int32 handleType; \
  ga_Mixer* mixer; \
  ga_FinishCallback callback; \
  void* context; \
  ga_int32 state; \
  ga_float32 gain; \
  ga_float32 pitch; \
  ga_float32 pan; \
  ga_int32 loop; \
  ga_int32 envDuration; \
  ga_float32 envGainA; \
  ga_float32 envGainB; \
  ga_int32 loopStart; \
  ga_int32 loopEnd; \
  ga_Link dispatchLink; \
  ga_Link mixLink; \
  ga_Link streamLink; \
  ga_Mutex* handleMutex;

typedef struct ga_Handle {
  GA_HANDLE_HEADER
} ga_Handle;

typedef struct ga_HandleStream ga_HandleStream;

#define GA_TELL_PARAM_CURRENT 0
#define GA_TELL_PARAM_TOTAL 1

typedef void (*ga_StreamProduceFunc)(ga_HandleStream* in_handle);
typedef void (*ga_StreamConsumeFunc)(ga_HandleStream* in_handle, ga_int32 in_samplesConsumed);
typedef void (*ga_StreamSeekFunc)(ga_HandleStream* in_handle, ga_int32 in_sampleOffset);
typedef ga_int32 (*ga_StreamTellFunc)(ga_HandleStream* in_handle, ga_int32 in_param);
typedef void (*ga_StreamDestroyFunc)(ga_HandleStream* in_handle);

#define GA_HANDLE_TYPE_UNKNOWN 0
#define GA_HANDLE_TYPE_STATIC 1
#define GA_HANDLE_TYPE_STREAM 2

typedef struct ga_HandleStatic {
  GA_HANDLE_HEADER
  ga_Sound* sound;
  ga_int32 nextSample;
} ga_HandleStatic;

typedef struct ga_HandleStream {
  GA_HANDLE_HEADER
  ga_StreamProduceFunc produceFunc;
  ga_StreamConsumeFunc consumeFunc;
  ga_StreamSeekFunc seekFunc;
  ga_StreamTellFunc tellFunc;
  ga_StreamDestroyFunc destroyFunc;
  void* streamContext;
  ga_int32 group;
  ga_CircBuffer* buffer;
  ga_Format format;
  ga_Mutex* consumeMutex;
} ga_HandleStream;

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_Sound* in_sound);
ga_Handle* ga_handle_createStream(ga_Mixer* in_mixer,
                                  ga_int32 in_group,
                                  ga_int32 in_bufferSize,
                                  ga_Format* in_format,
                                  ga_StreamProduceFunc in_produceFunc,
                                  ga_StreamConsumeFunc in_consumeFunc,
                                  ga_StreamSeekFunc in_seekFunc,
                                  ga_StreamTellFunc in_tellFunc,
                                  ga_StreamDestroyFunc in_destroyFunc,
                                  void* in_context);
ga_result ga_handle_destroy(ga_Handle* in_handle);
ga_result ga_handle_play(ga_Handle* in_handle);
ga_result ga_handle_stop(ga_Handle* in_handle);
ga_int32 ga_handle_finished(ga_Handle* in_handle);
ga_int32 ga_handle_destroyed(ga_Handle* in_handle);
ga_result ga_handle_setCallback(ga_Handle* in_handle,
                                ga_FinishCallback in_callback,
                                void* in_context);
ga_result ga_handle_setLoops(ga_Handle* in_handle,
                             ga_int32 in_loopStart, ga_int32 in_loopEnd);
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
ga_result ga_handle_seek(ga_Handle* in_handle, ga_int32 in_sampleOffset);
ga_int32 ga_handle_tell(ga_Handle* in_handle, ga_int32 in_param);

/*
  Gorilla Mixer
*/
typedef struct ga_Mixer {
  ga_Format format;
  ga_Format mixFormat;
  ga_int32 numSamples;
  ga_int32* mixBuffer;
  ga_Link dispatchList;
  ga_Mutex* dispatchMutex;
  ga_Link mixList;
  ga_Mutex* mixMutex;
  ga_Link streamList;
  ga_Mutex* streamMutex;
} ga_Mixer;

ga_Mixer* ga_mixer_create(ga_Format* in_format, ga_int32 in_numSamples);
ga_result ga_mixer_stream(ga_Mixer* in_mixer);
ga_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer);
ga_result ga_mixer_dispatch(ga_Mixer* in_mixer);
ga_result ga_mixer_destroy(ga_Mixer* in_mixer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
