#ifndef _GORILLA_GC_TYPES_H
#define _GORILLA_GC_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
3.4  Common Library (GC)

3.4.1  About Common Library

The common library (GC) is a collection of non-audio-specific classes that are common to most libraries.

3.4.2  Types (gc_types.h)

The GC library defines a number of cross-platform type definitions:

Unsigned integers: gc_uint8, gc_uint16, gc_uint32, gc_uint64
Signed integers: gc_int8, gc_int16, gc_int32, gc_int64
Floating-point numbers: gc_float32, gc_float64

There is also a result type with several values:

gc_result - A return type for the result of an operation.  Can be one of the following values:
GC_FALSE - The result was true
GC_TRUE - The result was false
GC_SUCCESS - The operation completed successfully
GC_ERROR_GENERIC The operation completed unsuccessfully with an unspecified error

3.4.3  System Operations (gc_common.h)

The GC library must be initialized prior to calling any other functions.  When finished using it, you must clean up the library.  When initializing the library, you may (optionally) pass in a gc_SystemOps structure to define custom allocation functions.  If you do not, Gorilla will use the standard ANSI C malloc/realloc/free functions.

System Operations (gc_SystemOps) - A POD object defining allocation behavior.

3.4.4  Containers (gc_common.h)

Circular Buffer (gc_CircBuffer) - A circular buffer object that is thread-safe for single producer/single consumer use cases.  NOTE: While it can be read/written from two different threads, the object’s memory management policy is Single Client, since there is only one owner responsible for creation/destruction of the thread.
Linked List (gc_Link) - A POD intrusive linked-list object.

3.4.5  Threads (gc_thread.h)

The GC library creates a thin platform-abstraction layer for threads.  When porting to a new platform, gc_Thread and gc_Mutex must both be ported to the new platform.

Thread (gc_Thread) - A single-client asynchronous system thread object.  Allows you to run a function on a separate thread.
Mutex (gc_Mutex) - A thread-synchronization mutual exclusion lock object.  Allows you to ensure that only a single thread enters the locked region at a time.  NOTE: While it can be locked/unlocked from several threads, the object’s memory management policy is Single Client, since there is only one owner responsible for creation/destruction of the mutex.

*/
/*
  WARNING: Do not typedef char or bool!
  (also, note that char != signed char != unsigned char)
  typedef char         char;
  typedef bool         bool;
*/

#ifdef _WIN32
  typedef unsigned char     gc_uint8;
  typedef unsigned short    gc_uint16;
  typedef unsigned int      gc_uint32;
  typedef unsigned __int64  gc_uint64;
  typedef signed char       gc_int8;
  typedef signed short      gc_int16;
  typedef signed int        gc_int32;
  typedef signed __int64    gc_int64;
  typedef float             gc_float32;
  typedef double            gc_float64;

#elif __GNUC__ /* GCC */
  typedef unsigned char          gc_uint8;
  typedef unsigned short         gc_uint16;
  typedef unsigned int           gc_uint32;
  typedef unsigned long long int gc_uint64;
  typedef signed char            gc_int8;
  typedef signed short           gc_int16;
  typedef signed int             gc_int32;
  typedef signed long long int   gc_int64;
  typedef float                  gc_float32;
  typedef double                 gc_float64;

#include <stdint.h>
#include <stddef.h>
#else
#error Types not yet specified for this platform
#endif

/*
  Error Codes
*/
typedef gc_int32 gc_result;

#define GC_FALSE 0
#define GC_TRUE 1
#define GC_SUCCESS 1
#define GC_ERROR_GENERIC -1

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GC_TYPES_H */
