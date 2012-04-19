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
ga_Handle* gau_stream_file(ga_Mixer* in_mixer,
                           const char* in_filename,
                           ga_int32 in_fileFormat,
                           ga_uint32 in_byteOffset,
                           ga_int32 in_group);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GAU_H */
