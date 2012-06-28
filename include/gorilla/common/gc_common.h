#ifndef _GORILLA_GC_COMMON_H
#define _GORILLA_GC_COMMON_H

#include "gc_types.h"
#include "gc_thread.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Gorilla System
*/
typedef struct gc_SystemOps {
  void* (*allocFunc)(gc_uint32 in_size);
  void* (*reallocFunc)(void* in_ptr, gc_uint32 in_size);
  void (*freeFunc)(void* in_ptr);
} gc_SystemOps;
extern gc_SystemOps* gcX_ops;

gc_result gc_initialize(gc_SystemOps* in_callbacks);
gc_result gc_shutdown();

/*
  Circular Buffer
*/
typedef struct gc_CircBuffer {
  gc_uint8* data;
  gc_uint32 dataSize;
  volatile gc_uint32 nextAvail;
  volatile gc_uint32 nextFree;
} gc_CircBuffer;

gc_CircBuffer* gc_buffer_create(gc_uint32 in_size);
gc_result gc_buffer_destroy(gc_CircBuffer* in_buffer);
gc_uint32 gc_buffer_bytesAvail(gc_CircBuffer* in_buffer);
gc_uint32 gc_buffer_bytesFree(gc_CircBuffer* in_buffer);
gc_result gc_buffer_getFree(gc_CircBuffer* in_buffer, gc_uint32 in_numBytes,
                            void** out_dataA, gc_uint32* out_sizeA,
                            void** out_dataB, gc_uint32* out_sizeB);
gc_result gc_buffer_write(gc_CircBuffer* in_buffer, void* in_data,
                          gc_uint32 in_numBytes);
gc_result gc_buffer_getAvail(gc_CircBuffer* in_buffer, gc_uint32 in_numBytes,
                             void** out_dataA, gc_uint32* out_sizeA,
                             void** out_dataB, gc_uint32* out_sizeB);
void gc_buffer_read(gc_CircBuffer* in_buffer, void* in_data,
                    gc_uint32 in_numBytes);
void gc_buffer_produce(gc_CircBuffer* in_buffer, gc_uint32 in_numBytes);
void gc_buffer_consume(gc_CircBuffer* in_buffer, gc_uint32 in_numBytes);

/*
  Linked List
*/
typedef struct gc_Link gc_Link;
typedef struct gc_Link {
  gc_Link* next;
  gc_Link* prev;
  void* data;
} gc_Link;

void gc_list_head(gc_Link* in_head);
void gc_list_link(gc_Link* in_head, gc_Link* in_link, void* in_data);
void gc_list_unlink(gc_Link* in_link);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GC_COMMON_H */
