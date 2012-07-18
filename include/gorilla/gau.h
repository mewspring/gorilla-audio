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

ga_Handle* gau_stream_file(ga_Mixer* in_mixer,
                           gc_int32 in_group,
                           gc_int32 in_bufferSize,
                           const char* in_filename,
                           gc_int32 in_fileFormat,
                           gc_uint32 in_byteOffset);

ga_Sound* gau_sound_file(const char* in_filename,
                         gc_int32 in_fileFormat,
                         gc_uint32 in_byteOffset);

ga_Handle* gau_stream_sample_source(ga_Mixer* in_mixer,
                                    gc_int32 in_group,
                                    gc_int32 in_bufferSize,
                                    ga_SampleSource* in_sampleSrc);

/* Concrete Implementations */
ga_DataSource* gau_data_source_create_file(const char* in_filename);
ga_DataSource* gau_data_source_create_file_arc(const char* in_filename, gc_int32 in_offset, gc_int32 in_size);
ga_SampleSource* gau_sample_source_create_wav(ga_DataSource* in_dataSrc);
ga_SampleSource* gau_sample_source_create_ogg(ga_DataSource* in_dataSrc);
ga_SampleSource* gau_sample_source_create_stream(ga_StreamManager* in_mgr, ga_SampleSource* in_sampleSrc, gc_int32 in_bufferSamples);
ga_SampleSource* gau_sample_source_create_sound(ga_Sound* in_sound);

/* Loop Sample Source */
typedef struct gau_SampleSourceLoop gau_SampleSourceLoop;
gau_SampleSourceLoop* gau_sample_source_create_loop(ga_SampleSource* in_sampleSrc);
void gau_sample_source_loop_set(gau_SampleSourceLoop* in_sampleSrc, gc_int32 in_triggerSample, gc_int32 in_targetSample);
void gau_sample_source_loop_clear(gau_SampleSourceLoop* in_sampleSrc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GAU_H */
