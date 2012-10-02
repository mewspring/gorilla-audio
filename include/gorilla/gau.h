#ifndef _GORILLA_GAU_H
#define _GORILLA_GAU_H

#include "common/gc_common.h"
#include "gorilla/ga.h"
#include "gorilla/ga_stream.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Gorilla Audio Utility Library
*/
  
/* High-Level Manager */
#define GAU_THREAD_POLICY_UNKNOWN 0
#define GAU_THREAD_POLICY_SINGLE 1
#define GAU_THREAD_POLICY_MULTI 2

typedef struct gau_Manager {
  gc_int32 threadPolicy;
  gc_Thread* mixThread;
  gc_Thread* streamThread;
  ga_Device* device;
  ga_Mixer* mixer;
  ga_StreamManager* streamMgr;
  gc_int32 sampleSize;
  gc_int16* mixBuffer;
  ga_Format format;
  gc_int32 killThreads;
} gau_Manager;

gau_Manager* gau_manager_create(gc_int32 in_devType,
                                gc_int32 in_threadPolicy,
                                gc_int32 in_numBuffers,
                                gc_int32 in_bufferSamples);
void gau_manager_update(gau_Manager* in_mgr);
ga_Mixer* gau_manager_mixer(gau_Manager* in_mgr);
ga_StreamManager* gau_manager_streamManager(gau_Manager* in_mgr);
ga_Device* gau_manager_device(gau_Manager* in_mgr);
void gau_manager_destroy(gau_Manager* in_mgr);

/* Concrete Source Implementations */
ga_DataSource* gau_data_source_create_file(const char* in_filename);
ga_DataSource* gau_data_source_create_file_arc(const char* in_filename, gc_int32 in_offset, gc_int32 in_size);
ga_SampleSource* gau_sample_source_create_wav(ga_DataSource* in_dataSrc);
ga_SampleSource* gau_sample_source_create_ogg(ga_DataSource* in_dataSrc);
ga_SampleSource* gau_sample_source_create_stream(ga_StreamManager* in_mgr, ga_SampleSource* in_sampleSrc, gc_int32 in_bufferSamples);
ga_SampleSource* gau_sample_source_create_sound(ga_Sound* in_sound);

/* Loop Source */
typedef struct gau_SampleSourceLoop gau_SampleSourceLoop;
gau_SampleSourceLoop* gau_sample_source_create_loop(ga_SampleSource* in_sampleSrc);
void gau_sample_source_loop_set(gau_SampleSourceLoop* in_sampleSrc, gc_int32 in_triggerSample, gc_int32 in_targetSample);
void gau_sample_source_loop_clear(gau_SampleSourceLoop* in_sampleSrc);

/* Helper functions */
ga_Sound* gau_helper_sound_file(const char* in_filename, const char* in_format);
ga_Handle* gau_helper_sound(ga_Mixer* in_mixer, ga_Sound* in_sound,
                            ga_FinishCallback in_callback, void* in_context,
                            gau_SampleSourceLoop** out_loopSrc);
ga_Handle* gau_helper_stream_data(ga_Mixer* in_mixer, ga_StreamManager* in_streamMgr, const char* in_format,
                                  ga_DataSource* in_dataSrc, ga_FinishCallback in_callback, void* in_context,
                                  gau_SampleSourceLoop** out_loopSrc);
ga_Handle* gau_helper_stream_file(ga_Mixer* in_mixer, ga_StreamManager* in_streamMgr,
                                  const char* in_filename, const char* in_format,
                                  ga_FinishCallback in_callback, void* in_context,
                                  gau_SampleSourceLoop** out_loopSrc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GAU_H */
