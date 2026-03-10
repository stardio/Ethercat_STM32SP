#include "oshw.h"
#include "osal.h"
#include <string.h>

uint16 oshw_htons(uint16 hostshort)
{
  return (uint16)((hostshort << 8) | (hostshort >> 8));
}

uint16 oshw_ntohs(uint16 networkshort)
{
  return oshw_htons(networkshort);
}

ec_adaptert *oshw_find_adapters(void)
{
  ec_adaptert *adapter = (ec_adaptert *)osal_malloc(sizeof(ec_adaptert));
  if (adapter == NULL)
  {
    return NULL;
  }
  memset(adapter, 0, sizeof(ec_adaptert));
  strncpy(adapter->name, "st_eth", EC_MAXLEN_ADAPTERNAME - 1);
  strncpy(adapter->desc, "STM32 ETH", EC_MAXLEN_ADAPTERNAME - 1);
  adapter->next = NULL;
  return adapter;
}

void oshw_free_adapters(ec_adaptert *adapter)
{
  while (adapter != NULL)
  {
    ec_adaptert *next = adapter->next;
    osal_free(adapter);
    adapter = next;
  }
}
