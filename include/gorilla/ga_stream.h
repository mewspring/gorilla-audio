/** Buffered Background Streaming.
 *
 *  \file ga_stream.h
 */

#ifndef _GORILLA_GA_STREAM_H
#define _GORILLA_GA_STREAM_H

#include "gorilla/ga.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Stream Manager */
typedef struct ga_StreamManager {
  gc_Link streamList;
  gc_Mutex* streamListMutex;
} ga_StreamManager;

ga_StreamManager* ga_stream_manager_create();
void ga_stream_manager_stream(ga_StreamManager* in_mgr);
void ga_stream_manager_destroy(ga_StreamManager* in_mgr);

/* Stream */
typedef struct ga_Stream {
  gc_Link* streamLink;
  ga_SampleSource* innerSrc;
  gc_CircBuffer* buffer;
  gc_Mutex* produceMutex;
  gc_Mutex* seekMutex;
  gc_Mutex* readMutex;
  gc_Mutex* refMutex;
  gc_int32 refCount;
  gc_Link tellJumps;
  ga_Format format;
  gc_int32 seek;
  gc_int32 tell;
  gc_int32 nextSample;
  gc_int32 end;
  gc_int32 flags;
  gc_int32 bufferSize;
} ga_Stream;

ga_Stream* ga_stream_create(ga_StreamManager* in_mgr, ga_SampleSource* in_sampleSource, gc_int32 in_bufferSize);
void ga_stream_produce(ga_Stream* in_stream); /* Can be called from a secondary thread */
gc_int32 ga_stream_read(ga_Stream* in_stream, void* in_dst, gc_int32 in_numSamples);
gc_int32 ga_stream_ready(ga_Stream* in_stream, gc_int32 in_numSamples);
gc_int32 ga_stream_end(ga_Stream* in_stream);
gc_int32 ga_stream_seek(ga_Stream* in_stream, gc_int32 in_sampleOffset);
gc_int32 ga_stream_tell(ga_Stream* in_stream, gc_int32* out_totalSamples);
gc_int32 ga_stream_flags(ga_Stream* in_stream);
void ga_stream_acquire(ga_Stream* in_stream);
void ga_stream_release(ga_Stream* in_stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
