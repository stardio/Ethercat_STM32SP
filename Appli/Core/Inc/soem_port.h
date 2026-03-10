#ifndef SOEM_PORT_H
#define SOEM_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SOEM_ENABLED
#include "soem/soem.h"

#ifndef SOEM_IFNAME
#define SOEM_IFNAME "st_eth"
#endif

extern ecx_contextt soem_context;

void SOEM_PortInit(void);
void SOEM_PortPoll(void);
void SOEM_PortSetLog(void (*log_fn)(const char *msg));
#else
static inline void SOEM_PortInit(void) {}
static inline void SOEM_PortPoll(void) {}
static inline void SOEM_PortSetLog(void (*log_fn)(const char *msg))
{
	(void)log_fn;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
