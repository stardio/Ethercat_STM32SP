#ifndef SOEM_PORT_H
#define SOEM_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef SOEM_ENABLED
#include "soem/soem.h"

#ifndef SOEM_IFNAME
#define SOEM_IFNAME "st_eth"
#endif

extern ecx_contextt soem_context;

void SOEM_PortInit(void);
void SOEM_PortPoll(void);
void SOEM_PortSetLog(void (*log_fn)(const char *msg));

/* PDO data accessors for UI (TouchGFX) */
int32_t SOEM_GetPositionActual(void);
int32_t SOEM_GetVelocityActual(void);
int16_t SOEM_GetTorqueActual(void);
uint16_t SOEM_GetStatusword(void);
uint8_t  SOEM_GetPdoReady(void);
uint8_t  SOEM_GetRunEnable(void);
void     SOEM_SetRunEnable(uint8_t enable);
void     SOEM_SetTargetPositionDelta(int32_t delta);
void     SOEM_SetTargetPositionAbs(int32_t pos);
void     SOEM_SetHomePosition(void);

#else
static inline void SOEM_PortInit(void) {}
static inline void SOEM_PortPoll(void) {}
static inline void SOEM_PortSetLog(void (*log_fn)(const char *msg))
{
	(void)log_fn;
}
static inline int32_t  SOEM_GetPositionActual(void) { return 0; }
static inline int32_t  SOEM_GetVelocityActual(void) { return 0; }
static inline int16_t  SOEM_GetTorqueActual(void)   { return 0; }
static inline uint16_t SOEM_GetStatusword(void)     { return 0; }
static inline uint8_t  SOEM_GetPdoReady(void)       { return 0; }
static inline uint8_t  SOEM_GetRunEnable(void)      { return 0; }
static inline void     SOEM_SetRunEnable(uint8_t enable) { (void)enable; }
static inline void     SOEM_SetTargetPositionDelta(int32_t delta) { (void)delta; }
static inline void     SOEM_SetTargetPositionAbs(int32_t pos)     { (void)pos; }
static inline void     SOEM_SetHomePosition(void)                  {}
#endif

#ifdef __cplusplus
}
#endif

#endif
