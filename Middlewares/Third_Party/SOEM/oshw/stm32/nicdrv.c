#include "oshw.h"
#include "osal.h"
#include "main.h"
#include <string.h>

#define SOEM_RX_BUF_SIZE 1524U

/* Debug counters - extern in main.c */
volatile uint32_t g_soem_tx_count = 0;
volatile uint32_t g_soem_tx_fail = 0;
volatile uint32_t g_soem_rx_count = 0;
volatile uint32_t g_soem_eth_state = 0;  /* ETH gState when TX fails */
volatile uint32_t g_soem_setupnic_state = 0;  /* ETH state in setupnic */
volatile uint32_t g_soem_hal_status = 0;  /* HAL return status (0=OK,1=ERR,2=BUSY,3=TIMEOUT) */
volatile uint32_t g_soem_eth_error = 0;  /* heth.ErrorCode */
volatile uint32_t g_soem_outframe_entry = 0;  /* ecx_outframe entry count */
volatile uint32_t g_soem_outframe_step = 0;  /* Last step reached in ecx_outframe */
volatile uint32_t g_soem_txbuflength = 0;  /* txbuflength value */

/** Redundancy modes */
enum
{
  ECT_RED_NONE,
  ECT_RED_DOUBLE
};

const uint16 priMAC[3] = EC_PRIMARY_MAC_ARRAY;
const uint16 secMAC[3] = EC_SECONDARY_MAC_ARRAY;

#define RX_PRIM priMAC[1]
#define RX_SEC  secMAC[1]

extern ETH_HandleTypeDef heth;

/* RX DMA pool must be non-cacheable to avoid stale frame reads on CPU side. */
__ALIGN_BEGIN static uint8 soem_rx_pool[ETH_RX_DESC_CNT][SOEM_RX_BUF_SIZE] __ALIGN_END __attribute__((section(".eth_dma"), aligned(32)));
static uint32 soem_rx_pool_idx = 0U;

static ETH_TxPacketConfigTypeDef soem_tx_config;
static ETH_BufferTypeDef soem_tx_buffer;

static void soem_mutex_lock(osal_mutext *mutex)
{
  if (mutex == NULL)
  {
    return;
  }
  for (;;)
  {
    uint32 primask = __get_PRIMASK();
    __disable_irq();
    if (mutex->lock == 0U)
    {
      mutex->lock = 1U;
      __set_PRIMASK(primask);
      return;
    }
    __set_PRIMASK(primask);
  }
}

static void soem_mutex_unlock(osal_mutext *mutex)
{
  if (mutex == NULL)
  {
    return;
  }
  uint32 primask = __get_PRIMASK();
  __disable_irq();
  mutex->lock = 0U;
  __set_PRIMASK(primask);
}

static void soem_cache_clean(const void *addr, uint32 length)
{
  uint32 start = ((uint32)addr) & ~31U;
  uint32 end = (((uint32)addr) + length + 31U) & ~31U;
  SCB_CleanDCache_by_Addr((uint32 *)start, (int32)(end - start));
  __DSB();  /* Memory barrier: ensure clean completes before DMA access */
  __ISB();  /* Instruction barrier: sync pipeline */
}

static void soem_cache_invalidate(const void *addr, uint32 length)
{
  uint32 start = ((uint32)addr) & ~31U;
  uint32 end = (((uint32)addr) + length + 31U) & ~31U;
  SCB_InvalidateDCache_by_Addr((uint32 *)start, (int32)(end - start));
  __DSB();  /* Memory barrier: ensure invalidate completes before CPU access */
  __ISB();  /* Instruction barrier: sync pipeline */
}

void HAL_ETH_RxAllocateCallback(uint8 **buff)
{
  *buff = soem_rx_pool[soem_rx_pool_idx];
  soem_rx_pool_idx = (soem_rx_pool_idx + 1U) % ETH_RX_DESC_CNT;
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8 *buff, uint16 Length)
{
  (void)Length;
  if (*pStart == NULL)
  {
    *pStart = buff;
  }
  *pEnd = buff;
}

static void ecx_clear_rxbufstat(int *rxbufstat)
{
  for (int i = 0; i < EC_MAXBUF; i++)
  {
    rxbufstat[i] = EC_BUF_EMPTY;
  }
}

void ec_setupheader(void *p)
{
  ec_etherheadert *bp = (ec_etherheadert *)p;
  bp->da0 = oshw_htons(0xffff);
  bp->da1 = oshw_htons(0xffff);
  bp->da2 = oshw_htons(0xffff);
  bp->sa0 = oshw_htons(priMAC[0]);
  bp->sa1 = oshw_htons(priMAC[1]);
  bp->sa2 = oshw_htons(priMAC[2]);
  bp->etype = oshw_htons(ETH_P_ECAT);
}

int ecx_setupnic(ecx_portt *port, const char *ifname, int secondary)
{
  (void)ifname;
  if (secondary)
  {
    return 0;
  }

  /* Initialize mutexes - CRITICAL for proper operation */
  port->getindex_mutex.lock = 0U;
  port->rx_mutex.lock = 0U;

  /* Record ETH state at entry */
  g_soem_setupnic_state = heth.gState;

  /* Force ETH to STARTED state */
  if (heth.gState != HAL_ETH_STATE_STARTED)
  {
    /* Stop first if in error state */
    if (heth.gState == HAL_ETH_STATE_ERROR)
    {
      HAL_ETH_Stop(&heth);
    }
    /* Start ETH */
    HAL_StatusTypeDef ret = HAL_ETH_Start(&heth);
    g_soem_setupnic_state = (g_soem_setupnic_state << 8) | (ret << 4) | heth.gState;
  }

  port->sockhandle = 1;
  port->lastidx = 0;
  port->redstate = ECT_RED_NONE;
  port->stack.sock = &(port->sockhandle);
  port->stack.txbuf = &(port->txbuf);
  port->stack.txbuflength = &(port->txbuflength);
  port->stack.tempbuf = &(port->tempinbuf);
  port->stack.rxbuf = &(port->rxbuf);
  port->stack.rxbufstat = &(port->rxbufstat);
  port->stack.rxsa = &(port->rxsa);
  ecx_clear_rxbufstat(&(port->rxbufstat[0]));

  for (int i = 0; i < EC_MAXBUF; i++)
  {
    ec_setupheader(&(port->txbuf[i]));
    port->rxbufstat[i] = EC_BUF_EMPTY;
  }
  ec_setupheader(&(port->txbuf2));

  soem_tx_config.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
  soem_tx_config.ChecksumCtrl = ETH_CHECKSUM_DISABLE;
  soem_tx_config.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  soem_tx_config.SrcAddrCtrl = ETH_SRC_ADDR_CONTROL_DISABLE;

  return 1;
}

int ecx_closenic(ecx_portt *port)
{
  port->sockhandle = -1;
  return 0;
}

uint8 ecx_getindex(ecx_portt *port)
{
  uint8 idx;
  uint8 cnt;

  soem_mutex_lock(&port->getindex_mutex);

  idx = (uint8)(port->lastidx + 1U);
  if (idx >= EC_MAXBUF)
  {
    idx = 0U;
  }
  cnt = 0U;
  while ((port->rxbufstat[idx] != EC_BUF_EMPTY) && (cnt < EC_MAXBUF))
  {
    idx++;
    cnt++;
    if (idx >= EC_MAXBUF)
    {
      idx = 0U;
    }
  }
  port->rxbufstat[idx] = EC_BUF_ALLOC;
  port->lastidx = idx;

  soem_mutex_unlock(&port->getindex_mutex);

  return idx;
}

void ecx_setbufstat(ecx_portt *port, uint8 idx, int bufstat)
{
  port->rxbufstat[idx] = bufstat;
}

int ecx_outframe(ecx_portt *port, uint8 idx, int stacknumber)
{
  g_soem_outframe_entry++;  /* Count every call to ecx_outframe */
  g_soem_outframe_step = 1;  /* Entry */
  
  if (stacknumber != 0)
  {
    g_soem_outframe_step = 2;  /* stacknumber != 0 */
    return 0;
  }

  g_soem_outframe_step = 3;  /* After stacknumber check */
  
  ec_stackT *stack = &(port->stack);
  if (stack == NULL || stack->txbuflength == NULL)
  {
    g_soem_outframe_step = 4;  /* NULL pointer */
    g_soem_tx_fail++;
    return EC_ERROR;
  }
  
  g_soem_outframe_step = 5;  /* Stack OK */
  
  int lp = (*stack->txbuflength)[idx];
  g_soem_txbuflength = (uint32_t)lp;
  
  if (lp <= 0)
  {
    g_soem_outframe_step = 6;  /* lp <= 0 */
    g_soem_tx_fail++;
    return EC_ERROR;
  }
  
  g_soem_outframe_step = 7;  /* lp OK */
  
  (*stack->rxbufstat)[idx] = EC_BUF_TX;

  soem_tx_buffer.buffer = (uint8_t *)(*stack->txbuf)[idx];
  soem_tx_buffer.len = (uint16_t)lp;
  soem_tx_buffer.next = NULL;
  soem_tx_config.Length = (uint32_t)lp;
  soem_tx_config.TxBuffer = &soem_tx_buffer;

  g_soem_outframe_step = 8;  /* Before cache clean */
  
  /* Clean TX buffer cache: CPU wrote data, DMA will read it */
  soem_cache_clean((*stack->txbuf)[idx], (uint32)lp);
  
  g_soem_outframe_step = 9;  /* Before HAL_ETH_Transmit */
  
  /* Additional memory barrier before DMA starts */
  __DSB();
  
  HAL_StatusTypeDef status = HAL_ETH_Transmit(&heth, &soem_tx_config, 100);
  
  g_soem_outframe_step = 10;  /* After HAL_ETH_Transmit */
  
  if (status != HAL_OK)
  {
    (*stack->rxbufstat)[idx] = EC_BUF_EMPTY;
    g_soem_tx_fail++;
    g_soem_eth_state = heth.gState;
    g_soem_hal_status = status;
    g_soem_eth_error = heth.ErrorCode;
    g_soem_outframe_step = 11;  /* TX failed */
    return EC_ERROR;
  }

  g_soem_tx_count++;
  g_soem_outframe_step = 12;  /* TX success */
  return lp;
}

int ecx_outframe_red(ecx_portt *port, uint8 idx)
{
  return ecx_outframe(port, idx, 0);
}

static int ecx_recvpkt(ecx_portt *port, int stacknumber)
{
  (void)stacknumber;
  void *appbuf = NULL;
  if (HAL_ETH_ReadData(&heth, &appbuf) != HAL_OK)
  {
    return 0;
  }
  port->tempinbufs = (int)heth.RxDescList.RxDataLength;
  if ((appbuf == NULL) || (port->tempinbufs <= 0))
  {
    return 0;
  }

  /* Invalidate DMA destination region before CPU reads appbuf. */
  soem_cache_invalidate(appbuf, (uint32)port->tempinbufs);

  g_soem_rx_count++;
  memcpy(port->tempinbuf, appbuf, (size_t)port->tempinbufs);
  return 1;
}

int ecx_inframe(ecx_portt *port, uint8 idx, int stacknumber)
{
  uint16 l;
  int rval;
  uint8 idxf;
  ec_etherheadert *ehp;
  ec_comt *ecp;
  ec_stackT *stack = &(port->stack);
  ec_bufT *rxbuf;

  (void)stacknumber;

  rval = EC_NOFRAME;
  rxbuf = &(*stack->rxbuf)[idx];
  if ((idx < EC_MAXBUF) && ((*stack->rxbufstat)[idx] == EC_BUF_RCVD))
  {
    l = (*rxbuf)[0] + ((uint16)((*rxbuf)[1] & 0x0f) << 8);
    rval = ((*rxbuf)[l] + ((uint16)(*rxbuf)[l + 1] << 8));
    (*stack->rxbufstat)[idx] = EC_BUF_COMPLETE;
  }
  else
  {
    soem_mutex_lock(&port->rx_mutex);
    if ((idx < EC_MAXBUF) && ((*stack->rxbufstat)[idx] == EC_BUF_RCVD))
    {
      l = (*rxbuf)[0] + ((uint16)((*rxbuf)[1] & 0x0f) << 8);
      rval = ((*rxbuf)[l] + ((uint16)(*rxbuf)[l + 1] << 8));
      (*stack->rxbufstat)[idx] = EC_BUF_COMPLETE;
    }
    else if (ecx_recvpkt(port, stacknumber))
    {
      rval = EC_OTHERFRAME;
      ehp = (ec_etherheadert *)(stack->tempbuf);
      if (ehp->etype == oshw_htons(ETH_P_ECAT))
      {
        stack->rxcnt++;
        ecp = (ec_comt *)(&(*stack->tempbuf)[ETH_HEADERSIZE]);
        l = etohs(ecp->elength) & 0x0fff;
        idxf = ecp->index;
        if (idxf == idx)
        {
          memcpy(rxbuf, &(*stack->tempbuf)[ETH_HEADERSIZE], (*stack->txbuflength)[idx] - ETH_HEADERSIZE);
          rval = ((*rxbuf)[l] + ((uint16)((*rxbuf)[l + 1]) << 8));
          (*stack->rxbufstat)[idx] = EC_BUF_COMPLETE;
          (*stack->rxsa)[idx] = oshw_ntohs(ehp->sa1);
        }
        else
        {
          if (idxf < EC_MAXBUF && (*stack->rxbufstat)[idxf] == EC_BUF_TX)
          {
            rxbuf = &(*stack->rxbuf)[idxf];
            memcpy(rxbuf, &(*stack->tempbuf)[ETH_HEADERSIZE], (*stack->txbuflength)[idxf] - ETH_HEADERSIZE);
            (*stack->rxbufstat)[idxf] = EC_BUF_RCVD;
            (*stack->rxsa)[idxf] = oshw_ntohs(ehp->sa1);
          }
        }
      }
    }
    soem_mutex_unlock(&port->rx_mutex);
  }

  return rval;
}

static int ecx_waitinframe_red(ecx_portt *port, uint8 idx, osal_timert *timer)
{
  int wkc = EC_NOFRAME;
  do
  {
    wkc = ecx_inframe(port, idx, 0);
  } while ((wkc <= EC_NOFRAME) && !osal_timer_is_expired(timer));

  return wkc;
}

int ecx_waitinframe(ecx_portt *port, uint8 idx, int timeout)
{
  osal_timert timer;
  osal_timer_start(&timer, (uint32)timeout);
  return ecx_waitinframe_red(port, idx, &timer);
}

int ecx_srconfirm(ecx_portt *port, uint8 idx, int timeout)
{
  int wkc = EC_NOFRAME;
  osal_timert timer1;
  osal_timert timer2;

  osal_timer_start(&timer1, (uint32)timeout);
  do
  {
    ecx_outframe_red(port, idx);
    if (timeout < EC_TIMEOUTRET)
    {
      osal_timer_start(&timer2, (uint32)timeout);
    }
    else
    {
      osal_timer_start(&timer2, EC_TIMEOUTRET);
    }
    wkc = ecx_waitinframe_red(port, idx, &timer2);
  } while ((wkc <= EC_NOFRAME) && !osal_timer_is_expired(&timer1));

  return wkc;
}
