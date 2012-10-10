/**
\file ga.h
Gorilla Audio API.
Data structures and functions for realtime audio playback.
*/
#ifndef _GORILLA_GA_H
#define _GORILLA_GA_H

#include "common/gc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**************************/
/*  Forward declarations  */
/**************************/
typedef struct ga_Handle ga_Handle;
typedef struct ga_Mixer ga_Mixer;

/*************/
/*  Version  */
/*************/
/** \defgroup version Version
API version and related functions.
*/
/** \ingroup version
\def GA_VERSION_MAJOR
Major version number.
Major version changes indicate a massive rewrite or refactor, and/or changes 
where API backwards-compatibility is greatly compromised. \ingroup version
*/
#define GA_VERSION_MAJOR 0

/** \ingroup version
\def GA_VERSION_MINOR
Minor version number.
Minor version changes indicate development milestones, large changes, and/or
changes where API backwards-compatibility is somewhat compromised.
*/
#define GA_VERSION_MINOR 2

/** \ingroup version
\def GA_VERSION_REV
Revision version number.
Revision version changes indicate a new version pushed to the trunk, and/or
changes where API backwards-compatibility is trivially compromised.
*/
#define GA_VERSION_REV 7

/** \ingroup version
Compares the API version against the specified version.
\param in_major Major version to compare against
\param in_minor Minor version to compare against
\param in_rev Revision version to compare against
\return 0 -> specified == api, 1 -> specified > api, -1 -> specified < api
*/
gc_int32 ga_version_check(gc_int32 in_major, gc_int32 in_minor, gc_int32 in_rev);

/***********************/
/*  Memory Management  */
/***********************/
/** \defgroup memManagement Memory Management
Memory management policies for different types of objects
*/
#define ABSTRACT /**< The object is abstract, and should never be instantiated directly.
                      The exception to this rule is when you are implementing a new concrete 
                      implementation. \ingroup memManagement */
#define POD /**< The object is POD (plain-ol'-data), and can be allocated/copied freely.
                 \ingroup memManagement */
#define SINGLE_CLIENT /**< The object has a single client (owner). The object should be
                           created/opened by its client, and then destroyed/closed when the
                           client is done with it. The object itself should never be copied.
                           Instead, a pointer to the object should be copied. The client 
                           must never use the object after destroying it. \ingroup memManagement */
#define MULTI_CLIENT /**< The object has multiple clients (owners), and is reference-counted.
                          The object should be created by its first client. Additional 
                          clients should call *_acquire() to add new references. Whenever a
                          client is done using the object, it should call *_release() to
                          remove its reference. When the last reference is removed, the 
                          object will be freed. A client must never use the object after 
                          releasing its reference. The object itself should never be copied.
                          Instead, a pointer to the object should be copied. \ingroup memManagement*/

/************/
/*  Format  */
/************/
/** \defgroup ga_Format Format
Audio format definition data structure and associated functions.
*/
/** \ingroup ga_Format
Audio format data structure (MEMORY_POD).
Stores the format (sample rate, bps, channels) for PCM audio data.
*/
typedef struct POD ga_Format {
  gc_int32 sampleRate; /**< Sample rate (usually 44100) */
  gc_int32 bitsPerSample; /**< Bits per PCM sample (usually 16) */
  gc_int32 numChannels; /**< Number of audio channels (1 for mono, 2 for stereo) */
} ga_Format;

/** \ingroup ga_Format
Retrives the sample size (in bytes) of a specified format
\ingroup devTypes
\param in_format Format of the PCM data
\return Sample size (in bytes) of the specified format
*/
gc_int32 ga_format_sampleSize(ga_Format* in_format);

/** \ingroup ga_Format
Converts a discrete number of PCM samples into the duration (in seconds) it will take to play back
\param in_format Format of PCM sample data
\param in_samples Number of PCM samples
\return Duration (in seconds) it will take to play back
*/
gc_float32 ga_format_toSeconds(ga_Format* in_format, gc_int32 in_samples);

/** \ingroup ga_Format
Converts a duration (in seconds) into the discrete number of PCM samples it will take to play for that long
\param in_format Format of PCM sample data
\param in_seconds Duration (in seconds)
\return Number of PCM samples it will take to play back for the given time
*/
gc_int32 ga_format_toSamples(ga_Format* in_format, gc_float32 in_seconds);

/************/
/*  Device  */
/************/
/** \defgroup ga_Device Device
Abstract device data structure and associated functions.
*/
#define GA_DEVICE_TYPE_DEFAULT -1 /**< Default device type (based on hard-coded priorities) \ingroup ga_Device */
#define GA_DEVICE_TYPE_UNKNOWN 0 /**< Unknown (invalid) device type \ingroup ga_Device */
#define GA_DEVICE_TYPE_OPENAL 1 /**< OpenAL playback device (Windows, Linux, Mac) \ingroup ga_Device */
#define GA_DEVICE_TYPE_DIRECTSOUND 2 /**< DirectSound playback device (Windows-only, disabled) \ingroup ga_Device */
#define GA_DEVICE_TYPE_XAUDIO2 3 /**< XAudio2 playback device (Windows-only) \ingroup ga_Device */

/** \ingroup ga_Device
Header of shared-data for all concrete device implementations.
Stores the device type, number of buffers, number of samples, and device PCM format.
*/
#define GA_DEVICE_HEADER gc_int32 devType; gc_int32 numBuffers; gc_int32 numSamples; ga_Format format;

/** \ingroup ga_Device
Hardware device abstract data structure [\ref SINGLE_CLIENT].
Abstracts the platform-specific details of presenting audio buffers to sound playback hardware.
\warning You can only have one device open at-a-time.
\warning Never instantiate a ga_Device directly, unless you are implementing a new concrete
         device implementation. Instead, you should use ga_device_open().
*/
typedef struct SINGLE_CLIENT ga_Device {
  GA_DEVICE_HEADER
} ga_Device;

/** \ingroup ga_Device
Opens a concrete audio device.
\param in_type Requested device type (usually GA_DEVICE_TYPE_DEFAULT).
\param in_numBuffers Requested number of buffers.
\param in_numSamples Requested sample buffer size.
\param in_format Requested device output format (usually 16-bit/44100/stereo).
\return Concrete instance of the requested device type. 0 if a suitable device could not be opened.
\todo Create a way to query the actual buffers/samples/format of the opened device.
*/
ga_Device* ga_device_open(gc_int32 in_type,
                          gc_int32 in_numBuffers,
                          gc_int32 in_numSamples,
                          ga_Format* in_format);

/** \ingroup ga_Device
Checks the number of free (unqueued) buffers.
\param in_device Device to check.
\return Number of free (unqueued) buffers.
*/
gc_int32 ga_device_check(ga_Device* in_device);

/** \ingroup ga_Device
Adds a buffer to a device's presentation queue.
\param in_device Device in which to queue the buffer.
\param in_buffer Buffer to add to the presentation queue.
\return GC_SUCCESS if the buffer was queued successfully. GC_ERROR_GENERIC if not.
\warning You should always call ga_device_check() prior to queueing a buffer! If 
         there isn't a free (unqueued) buffer, the operation will fail.
*/
gc_result ga_device_queue(ga_Device* in_device,
                          void* in_buffer);

/** \ingroup ga_Device
Closes an open audio device.
\param in_device Device to close.
\return GC_SUCCESS if the device was closed successfully. GC_ERROR_GENERIC if not.
*/
gc_result ga_device_close(ga_Device* in_device);

/************************/
/*  Global Definitions  */
/************************/
/** \defgroup globDefs Global Definitions
Enumerated seek origin values for use when seeking within data sources or sample sources.
*/
#define GA_SEEK_ORIGIN_SET 0 /**< Seek to an offset from the start of the source. \ingroup globDefs */
#define GA_SEEK_ORIGIN_CUR 1 /**< Seek to an offset from the current seek position. \ingroup globDefs */
#define GA_SEEK_ORIGIN_END 2 /**< Seek to an offset from the end of the source. \ingroup globDefs */

#define GA_FLAG_SEEKABLE 1 /**< Flag for sources that supports seeking. \ingroup globDefs */
#define GA_FLAG_THREADSAFE 2 /**< Flag for sources with a thread-safe interface. \ingroup globDefs */

/*****************/
/*  Data Source  */
/*****************/
/** \defgroup ga_DataSource Data Source
Abstract data source data structure and associated functions.
\todo Write an tutorial on how to write a ga_DataSource concrete implementation.
*/

/** \ingroup ga_DataSource
Data source read callback prototype.
\param in_context User context (pointer to the first byte after the data source).
\param in_dst Destination buffer into which bytes should be read. Guaranteed to
              be at least (in_size * in_count) bytes in size.
\param in_size Size of a single element (in bytes).
\param in_count Number of elements to read.
\return Total number of bytes read into the destination buffer.
*/
typedef gc_int32 (*tDataSourceFunc_Read)(void* in_context, void* in_dst, gc_int32 in_size, gc_int32 in_count);

/** \ingroup ga_DataSource
Data source seek callback prototype.
\param in_context User context (pointer to the first byte after the data source).
\param in_offset Offset (in bytes) from the specified seek origin.
\param in_origin Seek origin (see [\ref globDefs]).
\return If seek succeeds, the callback should return 0, otherwise it should return -1.
\warning Data sources with GA_FLAG_SEEKABLE should always provide a seek callback.
\warning Data sources with GA_FLAG_SEEKABLE set should only return -1 in the case of
         an invalid seek request.
\todo Define a less-confusing contract for extending/defining this function.
*/
typedef gc_int32 (*tDataSourceFunc_Seek)(void* in_context, gc_int32 in_offset, gc_int32 in_origin);

/** \ingroup ga_DataSource
Data source tell callback prototype.
\param in_context User context (pointer to the first byte after the data source).
\return The current data source read position.
*/
typedef gc_int32 (*tDataSourceFunc_Tell)(void* in_context);

/** \ingroup ga_DataSource
Data source close callback prototype.
\param in_context User context (pointer to the first byte after the data source).
*/
typedef void (*tDataSourceFunc_Close)(void* in_context);

/** \ingroup ga_DataSource
Abstract data source data structure [\ref ABSTRACT] [\ref MULTI_CLIENT].
A data source is a source of binary data, such as a file or socket, that 
generates bytes of binary data. This data is usually piped through a sample 
source to generate actual PCM audio data.
\todo Design a clearer/better system for easily extending this data type.
*/
typedef struct ABSTRACT MULTI_CLIENT ga_DataSource {
  tDataSourceFunc_Read readFunc; /**< Internal read callback. */
  tDataSourceFunc_Seek seekFunc; /**< Internal seek callback (optional). */
  tDataSourceFunc_Tell tellFunc; /**< Internal tell callback (optional). */
  tDataSourceFunc_Close closeFunc; /**< Internal close callback (optional). */
  gc_int32 refCount; /**< Reference count. */
  gc_Mutex* refMutex; /**< Mutex to protect reference count manipulations. */
  gc_int32 flags; /**< Flags defining which functionality this data source supports (see [\ref globDefs]). */
} ga_DataSource;

/** \ingroup ga_DataSource
Initializes the reference count and other default values.
Because ga_DataSource is an abstract data type, this function should not be 
called except when implement a concrete data source implementation.
\todo Hide this function (and the structure definition and callbacks) from the
      main user API headers. These details are only useful when extending data 
      sources.
*/
void ga_data_source_init(ga_DataSource* in_dataSrc);

/** \ingroup ga_DataSource
Reads binary data from the data source.
\param in_dataSrc Data source from which to read.
\param in_dst Destination buffer into which bytes should be read. Guaranteed to
be at least (in_size * in_count) bytes in size.
\param in_size Size of a single element (in bytes).
\param in_count Number of elements to read.
\return Total number of bytes read into the destination buffer.
*/
gc_int32 ga_data_source_read(ga_DataSource* in_dataSrc, void* in_dst, gc_int32 in_size, gc_int32 in_count);

/** \ingroup ga_DataSource
Seek to an offset within a data source.
\param in_dataSrc Data source to seek within.
\param in_offset Offset (in bytes) from the specified seek origin.
\param in_origin Seek origin (see [\ref globDefs]).
\return If seek succeeds, returns 0, otherwise returns -1 (invalid seek request).
\warning Only data sources with GA_FLAG_SEEKABLE can have ga_data_source_seek() called on them.
*/
gc_int32 ga_data_source_seek(ga_DataSource* in_dataSrc, gc_int32 in_offset, gc_int32 in_origin);

/** \ingroup ga_DataSource
Tells the current read position of a data source.
\param in_dataSrc Data source to tell the read position of.
\return The current data source read position.
*/
gc_int32 ga_data_source_tell(ga_DataSource* in_dataSrc);

/** \ingroup ga_DataSource
Returns the bitfield of flags set for a data source (see \ref globDefs).
\param in_dataSrc Data source whose flags should be retrieved.
\return The bitfield of flags set for the data source.
*/
gc_int32 ga_data_source_flags(ga_DataSource* in_dataSrc);

/** \ingroup ga_DataSource
Acquire a reference for a data source.
Increments the data source's reference count by 1.
\param in_dataSrc Data source whose reference count should be incremented.
*/
void ga_data_source_acquire(ga_DataSource* in_dataSrc);

/** \ingroup ga_DataSource
Releases a reference for a data source.
Decrements the data source's reference count by 1. When the last reference is
released, the data source's resources will be deallocated.
\param in_dataSrc Data source whose reference count should be decremented.
\warning A client must never use a data source after releasing its reference.
*/
void ga_data_source_release(ga_DataSource* in_dataSrc);

/*******************/
/*  Sample Source  */
/*******************/
/** \defgroup ga_SampleSource Sample Source
Abstract sample source data structure and associated functions.
*/
typedef void (*tOnSeekFunc)(gc_int32 in_sample, gc_int32 in_delta, void* in_seekContext);
typedef gc_int32 (*tSampleSourceFunc_Read)(void* in_context, void* in_dst, gc_int32 in_numSamples,
                                           tOnSeekFunc in_onSeekFunc, void* in_seekContext);
typedef gc_int32 (*tSampleSourceFunc_End)(void* in_context);
typedef gc_int32 (*tSampleSourceFunc_Ready)(void* in_context, gc_int32 in_numSamples);
typedef gc_int32 (*tSampleSourceFunc_Seek)(void* in_context, gc_int32 in_sampleOffset);
typedef gc_int32 (*tSampleSourceFunc_Tell)(void* in_context, gc_int32* out_totalSamples);
typedef void (*tSampleSourceFunc_Close)(void* in_context);

typedef struct ABSTRACT MULTI_CLIENT ga_SampleSource {
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

/************/
/*  Memory  */
/************/
/** \defgroup ga_Memory Memory
Reference-counted (shared) memory data structure and associated functions.
*/
/* [\ref MULTI_CLIENT] */
typedef struct ga_Memory {
  void* data;
  gc_uint32 size;
  gc_int32 refCount;
  gc_Mutex* refMutex;
} ga_Memory;

ga_Memory* ga_memory_create(void* in_data, gc_int32 in_size);
ga_Memory* ga_memory_create_data_source(ga_DataSource* in_dataSource);
gc_int32 ga_memory_size(ga_Memory* in_mem);
void* ga_memory_data(ga_Memory* in_mem);
void ga_memory_acquire(ga_Memory* in_mem);
void ga_memory_release(ga_Memory* in_mem);

/***********/
/*  Sound  */
/***********/
/** \defgroup ga_Sound Sound
Reference-counted (shared) static-sound data structure and associated functions.
*/
/* [\ref MULTI_CLIENT] */
typedef struct ga_Sound {
  ga_Memory* memory;
  ga_Format format;
  gc_int32 numSamples;
  gc_int32 refCount;
  gc_Mutex* refMutex;
} ga_Sound;

ga_Sound* ga_sound_create(ga_Memory* in_memory, ga_Format* in_format);
ga_Sound* ga_sound_create_sample_source(ga_SampleSource* in_sampleSrc);
void* ga_sound_data(ga_Sound* in_sound);
gc_int32 ga_sound_size(ga_Sound* in_sound);
gc_int32 ga_sound_numSamples(ga_Sound* in_sound);
void ga_sound_format(ga_Sound* in_sound, ga_Format* out_format);
void ga_sound_acquire(ga_Sound* in_sound);
void ga_sound_release(ga_Sound* in_sound);

/************/
/*  Handle  */
/************/
/** \defgroup ga_Handle Handle
Playback control handle data structure and associated functions.
Handles control pitch/pan/gain on playing audio, as well as let you play/pause/stop.
*/
#define GA_HANDLE_PARAM_UNKNOWN 0
#define GA_HANDLE_PARAM_PAN 1
#define GA_HANDLE_PARAM_PITCH 2
#define GA_HANDLE_PARAM_GAIN 3

#define GA_HANDLE_STATE_UNKNOWN 0
#define GA_HANDLE_STATE_INITIAL 1
#define GA_HANDLE_STATE_PLAYING 2
#define GA_HANDLE_STATE_STOPPED 3
#define GA_HANDLE_STATE_FINISHED 4
#define GA_HANDLE_STATE_DESTROYED 5

typedef void (*ga_FinishCallback)(ga_Handle*, void*);

/* [\ref SINGLE_CLIENT] */
struct ga_Handle {
  ga_Mixer* mixer;
  ga_FinishCallback callback;
  void* context;
  gc_int32 state;
  gc_float32 gain;
  gc_float32 pitch;
  gc_float32 pan;
  gc_Link dispatchLink;
  gc_Link mixLink;
  gc_Mutex* handleMutex;
  ga_SampleSource* sampleSrc;
  volatile gc_int32 finished;
};

#define GA_TELL_PARAM_CURRENT 0
#define GA_TELL_PARAM_TOTAL 1

ga_Handle* ga_handle_create(ga_Mixer* in_mixer, ga_SampleSource* in_sampleSrc);
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

/************/
/*  Mixer  */
/************/
/** \defgroup ga_Mixer Mixer
Multi-channel audio mixer data structure and associated functions.
*/
/* [\ref SINGLE_CLIENT] */
struct ga_Mixer {
  ga_Format format;
  ga_Format mixFormat;
  gc_int32 numSamples;
  gc_int32* mixBuffer;
  gc_Link dispatchList;
  gc_Mutex* dispatchMutex;
  gc_Link mixList;
  gc_Mutex* mixMutex;
};

ga_Mixer* ga_mixer_create(ga_Format* in_format, gc_int32 in_numSamples);
ga_Format* ga_mixer_format(ga_Mixer* in_mixer);
gc_int32 ga_mixer_numSamples(ga_Mixer* in_mixer);
gc_result ga_mixer_mix(ga_Mixer* in_mixer, void* out_buffer);
gc_result ga_mixer_dispatch(ga_Mixer* in_mixer);
gc_result ga_mixer_destroy(ga_Mixer* in_mixer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
