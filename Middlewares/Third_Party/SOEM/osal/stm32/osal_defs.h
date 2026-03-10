/*
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 */

#ifndef _osal_defs_
#define _osal_defs_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef EC_DEBUG
#include <stdio.h>
#define EC_PRINT printf
#else
#define EC_PRINT(...) \
  do                 \
  {                  \
  } while (0)
#endif

#ifndef OSAL_PACKED
#define OSAL_PACKED_BEGIN
#define OSAL_PACKED __attribute__((__packed__))
#define OSAL_PACKED_END
#endif

typedef struct
{
  int64_t tv_sec;
  int32_t tv_nsec;
} osal_timespec_t;

#define ec_timet osal_timespec_t

#define OSAL_THREAD_HANDLE  void *
#define OSAL_THREAD_FUNC    void
#define OSAL_THREAD_FUNC_RT void

typedef struct
{
  volatile uint32_t lock;
} osal_mutex_t;

#define osal_mutext osal_mutex_t

#ifdef __cplusplus
}
#endif

#endif
