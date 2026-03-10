/* ec_types.h - SOEM basic type definitions */
#ifndef _EC_TYPES_H_
#define _EC_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Basic types */
typedef uint8_t  uint8;
typedef int8_t   int8;
typedef uint16_t uint16;
typedef int16_t  int16;
typedef uint32_t uint32;
typedef int32_t  int32;
typedef uint64_t uint64;
typedef int64_t  int64;

typedef uint8 boolean;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EC_TYPES_H_ */
