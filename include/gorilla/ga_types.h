#ifndef _GORILLA_GA_TYPES_H
#define _GORILLA_GA_TYPES_H

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
  typedef unsigned char     ga_uint8;
  typedef unsigned short    ga_uint16;
  typedef unsigned int      ga_uint32;
  typedef unsigned __int64  ga_uint64;
  typedef signed char       ga_int8;
  typedef signed short      ga_int16;
  typedef signed int        ga_int32;
  typedef signed __int64    ga_int64;
  typedef float             ga_float32;
  typedef double            ga_float64;

#elif __GNUC__ /* GCC */
  typedef unsigned char          ga_uint8;
  typedef unsigned short         ga_uint16;
  typedef unsigned int           ga_uint32;
  typedef unsigned long long int ga_uint64;
  typedef signed char            ga_int8;
  typedef signed short           ga_int16;
  typedef signed int             ga_int32;
  typedef signed long long int   ga_int64;
  typedef float                  ga_float32;
  typedef double                 ga_float64;

#include <stdint.h>
#include <stddef.h>
#else
#error Types not yet specified for this platform
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GORILLA_GA_TYPES_H */
