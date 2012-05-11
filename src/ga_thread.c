#include "gorilla/ga.h"

#include "gorilla/ga_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Thread Functions */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static ga_int32 priorityLut[] = {
  0, -1, 1, 2
};

ga_Thread* ga_thread_create(ga_ThreadFunc in_threadFunc, void* in_context,
                            ga_int32 in_priority, ga_int32 in_stackSize)
{
  ga_Thread* ret = gaX_cb->allocFunc(sizeof(ga_Thread));
  ret->threadObj = gaX_cb->allocFunc(sizeof(HANDLE));
  ret->threadFunc = in_threadFunc;
  ret->context = in_context;
  ret->priority = in_priority;
  ret->stackSize = in_stackSize;
  *(HANDLE*)ret->threadObj = CreateThread(0, in_stackSize, (LPTHREAD_START_ROUTINE)in_threadFunc, in_context, CREATE_SUSPENDED, (LPDWORD)&ret->id);
  SetThreadPriority(*(HANDLE*)ret->threadObj, priorityLut[in_priority]);
  return ret;
}
void ga_thread_run(ga_Thread* in_thread)
{
  ResumeThread(*(HANDLE*)in_thread->threadObj);
}
void ga_thread_join(ga_Thread* in_thread)
{
  WaitForSingleObject(*(HANDLE*)in_thread->threadObj, INFINITE);
}
void ga_thread_sleep(ga_uint32 in_ms)
{
  Sleep(in_ms);
}
void ga_thread_destroy(ga_Thread* in_thread)
{
  CloseHandle(*(HANDLE*)in_thread->threadObj);
  gaX_cb->freeFunc(in_thread->threadObj);
  gaX_cb->freeFunc(in_thread);
}

#elif defined(__linux__)

#else
#error Thread class not yet defined for this platform
#endif /* _WIN32 */

/* Mutex Functions */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

ga_Mutex* ga_mutex_create()
{
  ga_Mutex* ret = gaX_cb->allocFunc(sizeof(ga_Mutex));
  ret->mutex = gaX_cb->allocFunc(sizeof(CRITICAL_SECTION));
  InitializeCriticalSection((CRITICAL_SECTION*)ret->mutex);
  return ret;
}
void ga_mutex_destroy(ga_Mutex* in_mutex)
{
  DeleteCriticalSection((CRITICAL_SECTION*)in_mutex->mutex);
  gaX_cb->freeFunc(in_mutex->mutex);
  gaX_cb->freeFunc(in_mutex);
}
void ga_mutex_lock(ga_Mutex* in_mutex)
{
  EnterCriticalSection((CRITICAL_SECTION*)in_mutex->mutex);
}
void ga_mutex_unlock(ga_Mutex* in_mutex)
{
  LeaveCriticalSection((CRITICAL_SECTION*)in_mutex->mutex);
}

#elif defined(__linux__)

#include <pthread.h>

ga_Mutex* ga_mutex_create()
{
  ga_Mutex* ret = gaX_cb->allocFunc(sizeof(ga_Mutex));
  ret->mutex = gaX_cb->allocFunc(sizeof(pthread_mutex_t));
  pthread_mutex_init((pthread_mutex_t*)ret->mutex, NULL);
  return ret;
}
void ga_mutex_destroy(ga_Mutex* in_mutex)
{
  pthread_mutex_destroy((pthread_mutex_t*)in_mutex->mutex);
  gaX_cb->freeFunc(in_mutex->mutex);
  gaX_cb->freeFunc(in_mutex);
}
void ga_mutex_lock(ga_Mutex* in_mutex)
{
  pthread_mutex_lock((pthread_mutex_t*)in_mutex->mutex);
}
void ga_mutex_unlock(ga_Mutex* in_mutex)
{
  pthread_mutex_unlock((pthread_mutex_t*)in_mutex->mutex);
}

#else
#error Mutex class not yet defined for this platform
#endif /* _WIN32 */
