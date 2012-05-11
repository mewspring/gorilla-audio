#ifndef _GORILLA_GAU_H
#define _GORILLA_GAU_H

#include "ga_types.h"

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
                           ga_int32 in_group,
                           ga_int32 in_bufferSize,
                           const char* in_filename,
                           ga_int32 in_fileFormat,
                           ga_uint32 in_byteOffset);

ga_Sound* gau_sound_file(const char* in_filename,
                        ga_int32 in_fileFormat,
                        ga_uint32 in_byteOffset);

ga_result gau_sound_file_format(const char* in_filename,
                                ga_int32 in_fileFormat,
                                ga_uint32 in_byteOffset,
                                ga_Format* out_format);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GAU_H */
