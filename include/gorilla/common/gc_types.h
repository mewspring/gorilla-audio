#ifndef _GORILLA_GC_TYPES_H
#define _GORILLA_GC_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

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
