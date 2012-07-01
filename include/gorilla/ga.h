#ifndef _GORILLA_GA_H
#define _GORILLA_GA_H

#include "common/gc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  API Version
*/
#define GA_VERSION_MAJOR 0
#define GA_VERSION_MINOR 1
#define GA_VERSION_REV 0

gc_int32 ga_version_check(gc_int32 in_major, gc_int32 in_minor, gc_int32 in_rev);

/*
  Forward Declarations
*/
typedef struct ga_Handle ga_Handle;
typedef struct ga_Mixer ga_Mixer;

/*
  Gorilla Audio Format
*/
typedef struct ga_Format {
  gc_int32 sampleRate;
  gc_int32 bitsPerSample;
  gc_int32 numChannels;
} ga_Format;

gc_int32 ga_format_sampleSize(ga_Format* in_format);
gc_float32 ga_format_toSeconds(ga_Format* in_format, gc_int32 in_samples);
gc_int32 ga_format_toSamples(ga_Format* in_format, gc_float32 in_seconds);

/*
  Gorilla Audio Device
*/
#define GA_DEVICE_TYPE_ANY -1
#define GA_DEVICE_TYPE_UNKNOWN 0
#define GA_DEVICE_TYPE_OPENAL 1

#define GA_DEVICE_HEADER gc_int32 devType;

typedef struct ga_Device {
  GA_DEVICE_HEADER
} ga_Device;

ga_Device* ga_device_open(gc_int32 in_type, gc_int32 in_numBuffers);
gc_int32 ga_device_check(ga_Device* in_device);
gc_result ga_device_queue(ga_Device* in_device,
                         ga_Format* in_format,
                         gc_int32 in_numSamples,
                         void* in_buffer);
gc_result ga_device_close(ga_Device* in_device);

/*
  Source Flags
*/
#define GA_FLAG_SEEKABLE 1
#define GA_FLAG_THREADSAFE 2

/*
  Data Source Structure
*/
/*
  DATASOURCE/SAMPLESOURCE MEMORY MANAGEMENT NOTES:
  ga_DataSource and ga_SampleSource use reference counting to manage memory.
  When creating a source, you must call *_source_init() (to init refcount).
  When you are finished using a source, call *_source_release().
  To add a new strong reference to the source, call *_source_acquire().
  To remove that strong reference, again, call *_source_release().
*/
#define GA_SEEK_ORIGIN_SET 0
#define GA_SEEK_ORIGIN_CUR 1
#define GA_SEEK_ORIGIN_END 2

typedef gc_int32 (*tDataSourceFunc_Read)(void* in_context, void* in_dst, gc_int32 in_size, gc_int32 in_count);
typedef gc_int32 (*tDataSourceFunc_Seek)(void* in_context, gc_int32 in_offset, gc_int32 in_origin);
typedef gc_int32 (*tDataSourceFunc_Tell)(void* in_context);
typedef void (*tDataSourceFunc_Close)(void* in_context);

typedef struct ga_DataSource {
  tDataSourceFunc_Read readFunc;
  tDataSourceFunc_Seek seekFunc; /* OPTIONAL */
  tDataSourceFunc_Tell tellFunc; /* OPTIONAL */
  tDataSourceFunc_Close closeFunc; /* OPTIONAL */
  gc_int32 refCount;
  gc_Mutex* refMutex;
  gc_int32 flags;
} ga_DataSource;

void ga_data_source_init(ga_DataSource* in_dataSrc);
gc_int32 ga_data_source_read(ga_DataSource* in_dataSrc, void* in_dst, gc_int32 in_size, gc_int32 in_count);
gc_int32 ga_data_source_seek(ga_DataSource* in_dataSrc, gc_int32 in_offset, gc_int32 in_origin);
gc_int32 ga_data_source_tell(ga_DataSource* in_dataSrc);
gc_int32 ga_data_source_flags(ga_DataSource* in_dataSrc);
void ga_data_source_acquire(ga_DataSource* in_dataSrc);
void ga_data_source_release(ga_DataSource* in_dataSrc);

/*
  Sample Source Structure
  */
typedef void (*tOnSeekFunc)(gc_int32 in_sample, gc_int32 in_delta, void* in_seekContext);
typedef gc_int32 (*tSampleSourceFunc_Read)(void* in_context, void* in_dst, gc_int32 in_numSamples,
                                           tOnSeekFunc in_onSeekFunc, void* in_seekContext);
typedef gc_int32 (*tSampleSourceFunc_End)(void* in_context);
typedef gc_int32 (*tSampleSourceFunc_Ready)(void* in_context, gc_int32 in_numSamples);
typedef gc_int32 (*tSampleSourceFunc_Seek)(void* in_context, gc_int32 in_sampleOffset);
typedef gc_int32 (*tSampleSourceFunc_Tell)(void* in_context, gc_int32* out_totalSamples);
typedef void (*tSampleSourceFunc_Close)(void* in_context);

typedef struct ga_SampleSource {
  tSampleSourceFunc_Read readFunc;
  tSampleSourceFunc_End endFunc;
  tSampleSourceFunc_Ready readyFunc;
  tSampleSourceFunc_Seek seekFunc; /* OPTIONAL */
  tSampleSourceFunc_Tell tellFunc; /* OPTIONAL */
  tSampleSourceFunc_Close closeFunc; /* OPTIONAL */
  ga_Format format;
  gc_int32 refCount;
  gc_Mutex* refMutex;
  gc_int32 flags;
} ga_SampleSource;

void ga_sample_source_init(ga_SampleSource* in_sampleSrc);
gc_int32 ga_sample_source_read(ga_SampleSource* in_sampleSrc, void* in_dst, gc_int32 in_numSamples,
                               tOnSeekFunc in_onSeekFunc, void* in_seekContext);
gc_int32 ga_sample_source_end(ga_SampleSource* in_sampleSrc);
gc_int32 ga_sample_source_ready(ga_SampleSource* in_sampleSrc, gc_int32 in_numSamples);
gc_int32 ga_sample_source_seek(ga_SampleSource* in_sampleSrc, gc_int32 in_sampleOffset);
gc_int32 ga_sample_source_tell(ga_SampleSource* in_sampleSrc, gc_int32* out_totalSamples);
gc_int32 ga_sample_source_flags(ga_SampleSource* in_sampleSrc);
void ga_sample_source_format(ga_SampleSource* in_sampleSrc, ga_Format* out_format);
void ga_sample_source_acquire(ga_SampleSource* in_sampleSrc);
void ga_sample_source_release(ga_SampleSource* in_sampleSrc);

/*
  Gorilla Sound
*/
typedef struct ga_Sound {
  gc_int32 loopStart;
  gc_int32 loopEnd;
  void* data;
  gc_uint32 size;
  ga_Format format;
  gc_int32 isCopy;
  gc_int32 numSamples;
} ga_Sound;

ga_Sound* ga_sound_create(void* in_data, gc_int32 in_size,
                          ga_Format* in_format, gc_int32 in_copy);
gc_result ga_sound_setLoops(ga_Sound* in_sound,
                            gc_int32 in_loopStart, gc_int32 in_loopEnd);
gc_int32 ga_sound_numSamples(ga_Sound* in_sound);
gc_result ga_sound_destroy(ga_Sound* in_sound);

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
  gc_int32 handleType; \
  ga_Mixer* mixer; \
  ga_FinishCallback callback; \
  void* context; \
  gc_int32 state; \
  gc_float32 gain; \
  gc_float32 pitch; \
  gc_float32 pan; \
  gc_int32 loop; \
  gc_int32 loopStart; \
  gc_int32 loopEnd; \
  gc_Link dispatchLink; \
  gc_Link mixLink; \
  gc_Link streamLink; \
  gc_Mutex* handleMutex;

typedef struct ga_Handle {
  GA_HANDLE_HEADER
} ga_Handle;

typedef struct ga_HandleStream ga_HandleStream;

#define GA_TELL_PARAM_CURRENT 0
#define GA_TELL_PARAM_TOTAL 1

typedef void (*ga_StreamProduceFunc)(ga_HandleStream* in_handle);
typedef void (*ga_StreamConsumeFunc)(ga_HandleStream* in_handle, gc_int32 in_samplesConsumed);
typedef void (*ga_StreamSeekFunc)(ga_HandleStream* in_handle, gc_int32 in_sampleOffset);
typedef gc_int32 (*ga_StreamTellFunc)(ga_HandleStream* in_handle, gc_int32 in_param);
typedef void (*ga_StreamDestroyFunc)(ga_HandleStream* in_handle);

#define GA_HANDLE_TYPE_UNKNOWN 0
#define GA_HANDLE_TYPE_STATIC 1
#define GA_HANDLE_TYPE_STREAM 2
#define GA_HANDLE_TYPE_SAMPLESOURCE 3

typedef struct ga_HandleStatic {
  GA_HANDLE_HEADER
  ga_Sound* sound;
  volatile gc_int32 nextSample;
} ga_HandleStatic;

typedef struct ga_HandleStream {
  GA_HANDLE_HEADER
  volatile ga_StreamProduceFunc produceFunc;
  ga_StreamConsumeFunc consumeFunc;
  ga_StreamSeekFunc seekFunc;
  ga_StreamTellFunc tellFunc;
  ga_StreamDestroyFunc destroyFunc;
  void* streamContext;
  gc_int32 group;
  gc_CircBuffer* buffer;
  ga_Format format;
  gc_Mutex* consumeMutex;
} ga_HandleStream;

typedef struct ga_HandleSampleSource {
  GA_HANDLE_HEADER
  ga_SampleSource* sampleSrc;
  volatile gc_int32 finished;
} ga_HandleSampleSource;

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_Sound* in_sound);
ga_Handle* ga_handle_createSampleSource(ga_Mixer* in_mixer,
                                        ga_SampleSource* in_sampleSrc);
ga_Handle* ga_handle_createStream(ga_Mixer* in_mixer,
                                  gc_int32 in_group,
                                  gc_int32 in_bufferSize,
                                  ga_Format* in_format,
                                  ga_StreamProduceFunc in_produceFunc,
                                  ga_StreamConsumeFunc in_consumeFunc,
                                  ga_StreamSeekFunc in_seekFunc,
                                  ga_StreamTellFunc in_tellFunc,
                                  ga_StreamDestroyFunc in_destroyFunc,
                                  void* in_context);
gc_result ga_handle_destroy(ga_Handle* in_handle);
gc_result ga_handle_play(ga_Handle* in_handle);
gc_result ga_handle_stop(ga_Handle* in_handle);
gc_int32 ga_handle_playing(ga_Handle* in_handle);
gc_int32 ga_handle_stopped(ga_Handle* in_handle);
gc_int32 ga_handle_finished(ga_Handle* in_handle);
gc_int32 ga_handle_destroyed(ga_Handle* in_handle);
gc_result ga_handle_setCallback(ga_Handle* in_handle,
                                ga_FinishCallback in_callback,
                                void* in_context);
gc_result ga_handle_setLoops(ga_Handle* in_handle,
                             gc_int32 in_loopStart, gc_int32 in_loopEnd);
gc_result ga_handle_setParamf(ga_Handle* in_handle, gc_int32 in_param,
                              gc_float32 in_value);
gc_result ga_handle_getParamf(ga_Handle* in_handle, gc_int32 in_param,
                              gc_float32* out_value);
gc_result ga_handle_setParami(ga_Handle* in_handle, gc_int32 in_param,
                              gc_int32 in_value);
gc_result ga_handle_getParami(ga_Handle* in_handle, gc_int32 in_param,
                              gc_int32* out_value);
gc_result ga_handle_seek(ga_Handle* in_handle, gc_int32 in_sampleOffset);
gc_int32 ga_handle_tell(ga_Handle* in_handle, gc_int32 in_param);
gc_int32 ga_handle_ready(ga_Handle* in_handle, gc_int32 in_numSamples);
void ga_handle_format(ga_Handle* in_handle, ga_Format* out_format);

/*
  Gorilla Mixer
*/
typedef struct ga_Mixer {
  ga_Format format;
  ga_Format mixFormat;
  gc_int32 numSamples;
  gc_int32* mixBuffer;
  gc_Link dispatchList;
  gc_Mutex* dispatchMutex;
  gc_Link mixList;
  gc_Mutex* mixMutex;
  gc_Link streamList;
  gc_Mutex* streamMutex;
} ga_Mixer;

ga_Mixer* ga_mixer_create(ga_Format* in_format, gc_int32 in_numSamples);
ga_Format* ga_mixer_format(ga_Mixer* in_mixer);
gc_int32 ga_mixer_numSamples(ga_Mixer* in_mixer);
gc_result ga_mixer_stream(ga_Mixer* in_mixer);
gc_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer);
gc_result ga_mixer_dispatch(ga_Mixer* in_mixer);
gc_result ga_mixer_destroy(ga_Mixer* in_mixer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
