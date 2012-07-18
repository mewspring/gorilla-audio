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
#define GA_FILE_FORMAT_UNKNOWN 0
#define GA_FILE_FORMAT_WAV 1
#define GA_FILE_FORMAT_OGG 2

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GAU_H */
