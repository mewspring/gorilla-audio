#ifndef _GORILLA_GA_THREAD_H
#define _GORILLA_GA_THREAD_H

#include "ga_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
  Gorilla Thread
*/
#define GA_THREAD_PRIORITY_NORMAL 0
#define GA_THREAD_PRIORITY_LOW 1
#define GA_THREAD_PRIORITY_HIGH 2

typedef ga_int32 (*ga_ThreadFunc)(void* in_context);

typedef struct ga_Thread {
  ga_ThreadFunc threadFunc;
  void* threadObj;
  void* context;
  ga_int32 id;
  ga_int32 priority;
  ga_int32 stackSize;
} ga_Thread;

ga_Thread* ga_thread_create(ga_ThreadFunc in_threadFunc, void* in_context,
                            ga_int32 in_priority, ga_int32 in_stackSize);
void ga_thread_run(ga_Thread* in_thread);
void ga_thread_join(ga_Thread* in_thread);
void ga_thread_sleep(ga_uint32 in_ms);
void ga_thread_destroy(ga_Thread* in_thread);

/*
  Gorilla Mutex
*/
typedef struct ga_Mutex {
  void* mutex;
} ga_Mutex;

ga_Mutex* ga_mutex_create();
void ga_mutex_lock(ga_Mutex* in_mutex);
void ga_mutex_unlock(ga_Mutex* in_mutex);
void ga_mutex_destroy(ga_Mutex* in_mutex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_H */
