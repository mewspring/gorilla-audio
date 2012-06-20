#ifndef _GORILLA_GC_THREAD_H
#define _GORILLA_GC_THREAD_H

#include "gc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Gorilla Thread
*/
#define GC_THREAD_PRIORITY_NORMAL 0
#define GC_THREAD_PRIORITY_LOW 1
#define GC_THREAD_PRIORITY_HIGH 2
#define GC_THREAD_PRIORITY_HIGHEST 3

typedef gc_int32 (*gc_ThreadFunc)(void* in_context);

typedef struct gc_Thread {
  gc_ThreadFunc threadFunc;
  void* threadObj;
  void* context;
  gc_int32 id;
  gc_int32 priority;
  gc_int32 stackSize;
} gc_Thread;

gc_Thread* gc_thread_create(gc_ThreadFunc in_threadFunc, void* in_context,
                            gc_int32 in_priority, gc_int32 in_stackSize);
void gc_thread_run(gc_Thread* in_thread);
void gc_thread_join(gc_Thread* in_thread);
void gc_thread_sleep(gc_uint32 in_ms);
void gc_thread_destroy(gc_Thread* in_thread);

/*
  Gorilla Mutex
*/
typedef struct gc_Mutex {
  void* mutex;
} gc_Mutex;

gc_Mutex* gc_mutex_create();
void gc_mutex_lock(gc_Mutex* in_mutex);
void gc_mutex_unlock(gc_Mutex* in_mutex);
void gc_mutex_destroy(gc_Mutex* in_mutex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GC_H */
