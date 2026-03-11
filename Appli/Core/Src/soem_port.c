#include "soem_port.h"

#ifdef SOEM_ENABLED

#include "soem/ec_main.h"
#include "soem/ec_type.h"
#include "stm32h7rsxx.h"  /* For SCB_CleanDCache_by_Addr, __DSB, __ISB */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef SOEM_IFNAME
#define SOEM_IFNAME "st_eth"
#endif

/* Forward declaration */
static void soem_log(const char *msg);

ecx_contextt soem_context;
/* Keep IOmap in AXI SRAM non-cacheable region (RAM_CMD) for ETH DMA coherency.
 * ETH DMA uses AXI bus — cannot access SRAMAHB (AHB-only). */
static uint8 soem_iomap[4096] __attribute__((section(".eth_dma"), aligned(32)));
static uint8 soem_group = 0;
static uint8 soem_initialized = 0;
static uint8 soem_configured = 0;
static uint8 soem_pdo_ready = 0;
static void (*soem_log_fn)(const char *msg) = NULL;

typedef struct __attribute__((packed))
{
  uint16 controlword;
  int32 target_position;
} soem_rxpdo_t;

typedef struct __attribute__((packed))
{
  uint16 statusword;
  int32 position_actual;
  int32 velocity_actual;
  int16 torque_actual;
} soem_txpdo_t;

static soem_rxpdo_t *soem_rxpdo = NULL;
static soem_txpdo_t *soem_txpdo = NULL;
static uint16 soem_last_statusword = 0xFFFFU;
static uint16 soem_last_controlword = 0xFFFFU;
static int32 soem_target_position = 0;
static uint8 soem_last_state = 0xFFU;

/* Volatile shadow copies for safe UI access (TouchGFX) */
static volatile int32_t  soem_shadow_position = 0;
static volatile int32_t  soem_shadow_velocity = 0;
static volatile int16_t  soem_shadow_torque   = 0;
static volatile uint16_t soem_shadow_statusword = 0;
static volatile uint8_t  soem_shadow_pdo_ready  = 0;
static volatile uint8_t  soem_shadow_run_enable = 0;

/* SOEM_PortPoll is called every ~100 ms in main task. */
#define SOEM_CIA402_HOLD_CYCLES 5U      /* ~500 ms */
#define SOEM_CIA402_TIMEOUT_CYCLES 50U  /* ~5 s */

typedef enum
{
  SOEM_CIA402_STAGE_SEND_SHUTDOWN = 0,
  SOEM_CIA402_STAGE_WAIT_READY,
  SOEM_CIA402_STAGE_SEND_SWITCH_ON,
  SOEM_CIA402_STAGE_WAIT_SWITCHED_ON,
  SOEM_CIA402_STAGE_SEND_ENABLE_OP,
  SOEM_CIA402_STAGE_WAIT_OPERATION_ENABLED,
  SOEM_CIA402_STAGE_OPERATION_ENABLED
} soem_cia402_stage_t;

static soem_cia402_stage_t soem_cia402_stage = SOEM_CIA402_STAGE_SEND_SHUTDOWN;
static uint16 soem_cia402_hold_counter = 0U;
static uint16 soem_cia402_timeout_counter = 0U;
static uint16 soem_last_sdo_statusword = 0xFFFFU;

/* Manual 1st PDO mapping: RxPDO=0x1600, TxPDO=0x1A00 */
#define SOEM_RXPDO_ASSIGN_INDEX 0x1600U
#define SOEM_TXPDO_ASSIGN_INDEX 0x1A00U
#define SOEM_DC_CYCLE_NS 100000000U /* 100 ms */
#define SOEM_DC_SHIFT_NS 0

static int soem_apply_manual_pdo_mapping(uint16 slave)
{
  int wkc;
  uint8 u8val;
  uint16 u16val;
  uint32 u32val;

  /* [Step 1] Clear SM2/SM3 assignment lists */
  u8val = 0;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C12, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM2 clear fail"); return 0; }
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C13, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM3 clear fail"); return 0; }
  soem_log("PDO: SM assignment cleared");

  /* Small delay for slave internal processing */
  for (volatile int d = 0; d < 500000; d++) { }

  /* [Step 2] Configure RxPDO 0x1600: Controlword(16) + Target Position(32) = 6 bytes */
  u8val = 0;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1600, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1600 sub0 clear fail"); return 0; }
  u32val = 0x60400010U;  /* Controlword, 16-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1600, 1, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1600 sub1 fail"); return 0; }
  u32val = 0x607A0020U;  /* Target Position, 32-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1600, 2, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1600 sub2 fail"); return 0; }
  u8val = 2;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1600, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1600 sub0 set fail"); return 0; }
  soem_log("PDO: RxPDO 0x1600 configured (6 bytes)");

  /* [Step 3] Configure TxPDO 0x1A00: Statusword(16) + Position(32) + Velocity(32) + Torque(16) = 12 bytes */
  u8val = 0;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub0 clear fail"); return 0; }
  u32val = 0x60410010U;  /* Statusword, 16-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 1, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub1 fail"); return 0; }
  u32val = 0x60640020U;  /* Actual Position, 32-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 2, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub2 fail"); return 0; }
  u32val = 0x606C0020U;  /* Actual Velocity, 32-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 3, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub3 fail"); return 0; }
  u32val = 0x60770010U;  /* Actual Torque, 16-bit */
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 4, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub4 fail"); return 0; }
  u8val = 4;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1A00, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: 0x1A00 sub0 set fail"); return 0; }
  soem_log("PDO: TxPDO 0x1A00 configured (12 bytes)");

  /* [Step 4] Assign PDOs to SM2/SM3 */
  u16val = 0x1600U;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C12, 1, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM2 assign fail"); return 0; }
  u16val = 0x1A00U;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C13, 1, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM3 assign fail"); return 0; }

  /* Small delay before activation */
  for (volatile int d = 0; d < 500000; d++) { }

  /* [Step 5] Activate: set count = 1 */
  u8val = 1;
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C12, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM2 activate fail"); return 0; }
  wkc = ecx_SDOwrite(&soem_context, slave, 0x1C13, 0, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTRXM);
  if (wkc <= 0) { soem_log("PDO: SM3 activate fail"); return 0; }

  soem_log("PDO: Manual mapping complete (Rx=6B, Tx=12B)");
  return 1;
}

static uint16 soem_read_statusword_sdo(uint16 slave, uint8 *ok)
{
  uint16 statusword = 0U;
  int size = (int)sizeof(statusword);
  int wkc = ecx_SDOread(&soem_context, slave, 0x6041, 0, FALSE, &size, &statusword, EC_TIMEOUTRXM);

  if ((wkc > 0) && (size >= (int)sizeof(statusword)))
  {
    *ok = 1U;
    return statusword;
  }

  *ok = 0U;
  return 0U;
}

static uint8 soem_decode_cia402_state(uint16 statusword)
{
  uint16 masked = (uint16)(statusword & 0x006FU);

  switch (masked)
  {
    case 0x0000U:
      return 0U; /* Not ready to switch on */
    case 0x0040U:
      return 1U; /* Switch on disabled */
    case 0x0021U:
      return 2U; /* Ready to switch on */
    case 0x0023U:
      return 3U; /* Switched on */
    case 0x0027U:
      return 4U; /* Operation enabled */
    case 0x0007U:
      return 5U; /* Quick stop active */
    case 0x000FU:
      return 6U; /* Fault reaction active */
    case 0x0008U:
      return 7U; /* Fault */
    default:
      return 255U; /* Unknown */
  }
}

static void soem_log(const char *msg)
{
  if (soem_log_fn != NULL)
  {
    size_t n = strlen(msg);
    if ((n > 0U) && ((msg[n - 1U] == '\n') || (msg[n - 1U] == '\r')))
    {
      soem_log_fn(msg);
    }
    else
    {
      char line[192];
      (void)snprintf(line, sizeof(line), "%s\r\n", msg);
      soem_log_fn(line);
    }
  }
}

static void soem_log_cia402_state(uint8 state)
{
  switch (state)
  {
    case 0U:
      soem_log("CIA402: Not ready to switch on");
      break;
    case 1U:
      soem_log("CIA402: Switch on disabled");
      break;
    case 2U:
      soem_log("CIA402: Ready to switch on");
      break;
    case 3U:
      soem_log("CIA402: Switched on");
      break;
    case 4U:
      soem_log("CIA402: Operation enabled");
      break;
    case 5U:
      soem_log("CIA402: Quick stop active");
      break;
    case 6U:
      soem_log("CIA402: Fault reaction active");
      break;
    case 7U:
      soem_log("CIA402: Fault");
      break;
    default:
      soem_log("CIA402: Unknown state");
      break;
  }
}

static void soem_update_pdo_pointers(void)
{
  ec_groupt *grp = soem_context.grouplist + soem_group;
  char line[96];
  soem_rxpdo = NULL;
  soem_txpdo = NULL;
  soem_pdo_ready = 0U;

  if ((grp->outputs != NULL) && (grp->Obytes >= (int)sizeof(soem_rxpdo_t)))
  {
    soem_rxpdo = (soem_rxpdo_t *)grp->outputs;
  }

  if ((grp->inputs != NULL) && (grp->Ibytes >= (int)sizeof(soem_txpdo_t)))
  {
    soem_txpdo = (soem_txpdo_t *)grp->inputs;
  }

  if ((soem_rxpdo != NULL) && (soem_txpdo != NULL))
  {
    soem_pdo_ready = 1U;
    (void)snprintf(line, sizeof(line), "SOEM: PDO ready Obytes=%lu Ibytes=%lu",
             (unsigned long)grp->Obytes, (unsigned long)grp->Ibytes);
    soem_log(line);
    (void)snprintf(line, sizeof(line), "SOEM: IOmap=0x%08lX O=0x%08lX I=0x%08lX",
             (unsigned long)(uintptr_t)soem_iomap,
             (unsigned long)(uintptr_t)grp->outputs,
             (unsigned long)(uintptr_t)grp->inputs);
    soem_log(line);
    soem_log("SOEM: PDO ready");
  }
  else
  {
    (void)snprintf(line, sizeof(line), "SOEM: PDO mismatch Obytes=%lu Ibytes=%lu",
             (unsigned long)grp->Obytes, (unsigned long)grp->Ibytes);
    soem_log(line);
    (void)snprintf(line, sizeof(line), "SOEM: IOmap=0x%08lX O=0x%08lX I=0x%08lX",
             (unsigned long)(uintptr_t)soem_iomap,
             (unsigned long)(uintptr_t)grp->outputs,
             (unsigned long)(uintptr_t)grp->inputs);
    soem_log(line);
    soem_log("SOEM: PDO size mismatch");
  }
}

static void soem_cia402_step(uint16 statusword)
{
  uint16 effective_statusword = statusword;
  uint16 controlword = soem_last_controlword;

  /* If PDO statusword is still zero, skip CiA402 processing —
   * the slave hasn't started sending TxPDO yet.  Do NOT use SDO
   * fallback here because SDO frames inside the cyclic PDO loop
   * corrupt the ETH DMA state and cause HardFault. */
  if (statusword == 0U)
  {
    static uint16 zero_count = 0U;
    zero_count++;
    if ((zero_count % 10U) == 1U)
    {
      soem_log("CIA402: waiting for PDO statusword");
    }
    /* Keep sending controlword=0 (no action) while waiting. */
    soem_rxpdo->controlword = 0U;
    soem_rxpdo->target_position = soem_target_position;
    return;
  }

  /* RUN/STOP direct gate:
   * RUN=0 -> hold at Shutdown command (0x0006), never auto-progress to Enable Op.
   * RUN=1 -> allow full CiA402 sequence to Operation Enabled.
   */
  if (soem_shadow_run_enable == 0U)
  {
    controlword = 0x0006U;
    soem_cia402_stage = SOEM_CIA402_STAGE_WAIT_READY;
    soem_cia402_hold_counter = 0U;
    soem_cia402_timeout_counter = SOEM_CIA402_TIMEOUT_CYCLES;

    if (controlword != soem_last_controlword)
    {
      char line[64];
      soem_last_controlword = controlword;
      (void)snprintf(line, sizeof(line), "CIA402: CW=0x%04X", controlword);
      soem_log(line);
    }

    soem_rxpdo->controlword = controlword;
    soem_rxpdo->target_position = soem_target_position;
    return;
  }

  /* Fault bit set: send fault reset first. */
  if ((effective_statusword & 0x0008U) != 0U)
  {
    controlword = 0x0080U;
    soem_cia402_stage = SOEM_CIA402_STAGE_SEND_SHUTDOWN;
    soem_cia402_hold_counter = 0U;
    soem_cia402_timeout_counter = 0U;
    soem_log("CIA402: Fault reset (0x0080)");
  }
  else
  {
    switch (soem_cia402_stage)
    {
      case SOEM_CIA402_STAGE_SEND_SHUTDOWN:
        controlword = 0x0006U;
        soem_cia402_hold_counter = SOEM_CIA402_HOLD_CYCLES;
        soem_cia402_timeout_counter = SOEM_CIA402_TIMEOUT_CYCLES;
        soem_cia402_stage = SOEM_CIA402_STAGE_WAIT_READY;
        soem_log("CIA402: Shutdown (0x0006)");
        break;

      case SOEM_CIA402_STAGE_WAIT_READY:
        controlword = 0x0006U;
        if (soem_cia402_hold_counter > 0U)
        {
          soem_cia402_hold_counter--;
          break;
        }

        /* Accept L7NH-ready variants: 0x21 or 0x31 (masked to 0x21). */
        if ((effective_statusword & 0x006FU) == 0x0021U)
        {
          soem_cia402_stage = SOEM_CIA402_STAGE_SEND_SWITCH_ON;
        }
        else if (soem_cia402_timeout_counter > 0U)
        {
          soem_cia402_timeout_counter--;
        }
        else
        {
          soem_log("CIA402: Timeout waiting ready, retry shutdown");
          soem_cia402_stage = SOEM_CIA402_STAGE_SEND_SHUTDOWN;
        }
        break;

      case SOEM_CIA402_STAGE_SEND_SWITCH_ON:
        controlword = 0x0007U;
        soem_cia402_hold_counter = SOEM_CIA402_HOLD_CYCLES;
        soem_cia402_timeout_counter = SOEM_CIA402_TIMEOUT_CYCLES;
        soem_cia402_stage = SOEM_CIA402_STAGE_WAIT_SWITCHED_ON;
        soem_log("CIA402: Switch On (0x0007)");
        break;

      case SOEM_CIA402_STAGE_WAIT_SWITCHED_ON:
        controlword = 0x0007U;
        if (soem_cia402_hold_counter > 0U)
        {
          soem_cia402_hold_counter--;
          break;
        }

        /* Accept L7NH-switched-on variants: 0x23 or 0x33 (masked to 0x23). */
        if ((effective_statusword & 0x006FU) == 0x0023U)
        {
          soem_cia402_stage = SOEM_CIA402_STAGE_SEND_ENABLE_OP;
        }
        else if (soem_cia402_timeout_counter > 0U)
        {
          soem_cia402_timeout_counter--;
        }
        else
        {
          soem_log("CIA402: Timeout waiting switched-on, retry switch-on");
          soem_cia402_stage = SOEM_CIA402_STAGE_SEND_SWITCH_ON;
        }
        break;

      case SOEM_CIA402_STAGE_SEND_ENABLE_OP:
        controlword = 0x000FU;
        soem_cia402_hold_counter = SOEM_CIA402_HOLD_CYCLES;
        soem_cia402_timeout_counter = SOEM_CIA402_TIMEOUT_CYCLES;
        soem_cia402_stage = SOEM_CIA402_STAGE_WAIT_OPERATION_ENABLED;
        soem_log("CIA402: Enable Op (0x000F)");
        break;

      case SOEM_CIA402_STAGE_WAIT_OPERATION_ENABLED:
        controlword = 0x000FU;
        if (soem_cia402_hold_counter > 0U)
        {
          soem_cia402_hold_counter--;
          break;
        }

        /* Accept L7NH-op-enabled variants: 0x27 or 0x37 (masked to 0x27). */
        if ((effective_statusword & 0x006FU) == 0x0027U)
        {
          soem_cia402_stage = SOEM_CIA402_STAGE_OPERATION_ENABLED;
          soem_log("CIA402: Operation enabled confirmed");
        }
        else if (soem_cia402_timeout_counter > 0U)
        {
          soem_cia402_timeout_counter--;
        }
        else
        {
          soem_log("CIA402: Timeout waiting op-enabled, retry enable-op");
          soem_cia402_stage = SOEM_CIA402_STAGE_SEND_ENABLE_OP;
        }
        break;

      case SOEM_CIA402_STAGE_OPERATION_ENABLED:
      default:
        controlword = 0x000FU;
        break;
    }
  }

  if (controlword != soem_last_controlword)
  {
    char line[64];
    soem_last_controlword = controlword;
    (void)snprintf(line, sizeof(line), "CIA402: CTL=0x%04X", controlword);
    soem_log(line);
  }

  soem_rxpdo->controlword = controlword;
  soem_rxpdo->target_position = soem_target_position;
}

void SOEM_PortSetLog(void (*log_fn)(const char *msg))
{
  soem_log_fn = log_fn;
}

void SOEM_PortInit(void)
{
  if (soem_initialized != 0U)
  {
    return;
  }
  if (ecx_init(&soem_context, SOEM_IFNAME) > 0)
  {
    soem_initialized = 1U;
    soem_log("SOEM: init ok");
  }
  else
  {
    soem_log("SOEM: init failed");
  }
}

void SOEM_PortPoll(void)
{
  if (soem_initialized == 0U)
  {
    return;
  }
  if (soem_configured == 0U)
  {
    static uint8 config_phase = 0;  /* 0=init, 1=wait_safeop, 2=wait_op */
    static uint16 phase_counter = 0;

    switch (config_phase)
    {
      case 0: /* --- INIT: config + request SAFE_OP --- */
      {
        char log_line[128];
        int slavecount = ecx_config_init(&soem_context);
        if (slavecount <= 0)
        {
          soem_log("SOEM: no slaves");
          return;
        }

        /* Force INIT → PRE_OP transition (SDO/mailbox requires PRE_OP) */
        soem_log("SOEM: forcing PRE_OP");
        soem_context.slavelist[0].state = EC_STATE_INIT;
        ecx_writestate(&soem_context, 0);
        for (volatile int d = 0; d < 1000000; d++) { }

        soem_context.slavelist[0].state = EC_STATE_PRE_OP;
        ecx_writestate(&soem_context, 0);

        /* Wait for PRE_OP (poll up to ~2 seconds) */
        {
          int preop_wait = 0;
          while (preop_wait < 20)
          {
            for (volatile int d = 0; d < 1000000; d++) { }
            ecx_statecheck(&soem_context, 0, EC_STATE_PRE_OP, 100000);
            ecx_readstate(&soem_context);
            uint16 st = soem_context.slavelist[1].state & 0x0FU;
            if (st >= EC_STATE_PRE_OP)
            {
              (void)snprintf(log_line, sizeof(log_line),
                             "SOEM: PRE_OP reached (try %d) state=0x%02X",
                             preop_wait, soem_context.slavelist[1].state);
              soem_log(log_line);
              break;
            }
            preop_wait++;
          }
          if (preop_wait >= 20)
          {
            (void)snprintf(log_line, sizeof(log_line),
                           "SOEM: PRE_OP timeout state=0x%02X",
                           soem_context.slavelist[1].state);
            soem_log(log_line);
            return;
          }
        }

        /* ACK any error state so slave is in clean PRE_OP */
        if ((soem_context.slavelist[1].state & 0x10U) != 0U)
        {
          soem_log("SOEM: Error ACK");
          soem_context.slavelist[0].state = (EC_STATE_PRE_OP | EC_STATE_ERROR);
          ecx_writestate(&soem_context, 0);
          for (volatile int d = 0; d < 1000000; d++) { }
          soem_context.slavelist[0].state = EC_STATE_PRE_OP;
          ecx_writestate(&soem_context, 0);
          for (volatile int d = 0; d < 500000; d++) { }
        }

        /* Manual PDO mapping BEFORE ec_config_map (matches working Linux code) */
        soem_log("SOEM: manual PDO mapping");
        if (soem_apply_manual_pdo_mapping(1) == 0)
        {
          soem_log("SOEM: manual PDO mapping FAILED");
          return;
        }

        /* Now let SOEM read the updated PDO assignments and configure SM */
        soem_log("SOEM: config_map_group");
        ecx_config_map_group(&soem_context, soem_iomap, soem_group);

        soem_update_pdo_pointers();

        /* Dump SOEM's SM configuration for slave 1 */
        {
          ec_slavet *sl = &soem_context.slavelist[1];
          (void)snprintf(log_line, sizeof(log_line),
                         "SM2: addr=0x%04X len=%u flags=0x%08lX",
                         sl->SM[2].StartAddr, sl->SM[2].SMlength,
                         (unsigned long)sl->SM[2].SMflags);
          soem_log(log_line);
          (void)snprintf(log_line, sizeof(log_line),
                         "SM3: addr=0x%04X len=%u flags=0x%08lX",
                         sl->SM[3].StartAddr, sl->SM[3].SMlength,
                         (unsigned long)sl->SM[3].SMflags);
          soem_log(log_line);
          (void)snprintf(log_line, sizeof(log_line),
                         "SOEM: Obits=%u Ibits=%u Obytes=%lu Ibytes=%lu",
                         sl->Obits, sl->Ibits,
                         (unsigned long)sl->Obytes, (unsigned long)sl->Ibytes);
          soem_log(log_line);
        }

        /* Set operation mode to CSP (Cyclic Synchronous Position) */
        {
          int8 mode = 8;
          (void)ecx_SDOwrite(&soem_context, 1, 0x6060, 0, FALSE, sizeof(mode),
                             &mode, EC_TIMEOUTRXM);
          soem_log("CIA402: Mode=CSP(8)");
        }

        soem_log("SOEM: requesting SAFE_OP");
        soem_context.slavelist[0].state = EC_STATE_SAFE_OP;
        ecx_writestate(&soem_context, 0);

        config_phase = 1;
        phase_counter = 0;
        return;
      }

      case 1: /* --- WAIT SAFE_OP (non-blocking, one check per poll) --- */
      {
        (void)ecx_send_processdata(&soem_context);
        (void)ecx_receive_processdata(&soem_context, EC_TIMEOUTRET);

        ecx_readstate(&soem_context);
        uint16 sl_state = soem_context.slavelist[1].state & 0x0FU;
        uint16 sl_al = soem_context.slavelist[1].ALstatuscode;
        phase_counter++;

        if (sl_state >= EC_STATE_SAFE_OP)
        {
          char msg[96];
          (void)snprintf(msg, sizeof(msg),
                         "SOEM: SAFE_OP reached (cycle %u) AL=0x%04X",
                         phase_counter, sl_al);
          soem_log(msg);

          /* PDO burst + OP request + wait in tight loop */
          soem_log("SOEM: PDO burst + OP transition");
          for (int burst = 0; burst < 200; burst++)
          {
            (void)ecx_send_processdata(&soem_context);
            (void)ecx_receive_processdata(&soem_context, EC_TIMEOUTRET);
            for (volatile int d = 0; d < 100000; d++) { } /* ~1ms delay */

            /* Request OP after first 50 bursts */
            if (burst == 50)
            {
              soem_log("SOEM: requesting OPERATIONAL");
              soem_context.slavelist[0].state = EC_STATE_OPERATIONAL;
              ecx_writestate(&soem_context, 0);
            }

            /* Check if OP reached */
            if (burst > 50 && (burst % 10) == 0)
            {
              ecx_readstate(&soem_context);
              if (soem_context.slavelist[1].state == EC_STATE_OPERATIONAL)
              {
                soem_configured = 1U;
                soem_log("[SOEM] OPERATIONAL reached in burst");
                break;
              }
            }
          }

          if (soem_configured == 0U)
          {
            /* Fall through to phase 2 polling */
            ecx_readstate(&soem_context);
            char m2[96];
            (void)snprintf(m2, sizeof(m2),
                           "SOEM: burst done state=0x%02X AL=0x%04X",
                           soem_context.slavelist[1].state,
                           soem_context.slavelist[1].ALstatuscode);
            soem_log(m2);
          }

          config_phase = 2;
          phase_counter = 0;
        }
        else if ((phase_counter % 10U) == 0U)
        {
          char msg[96];
          (void)snprintf(msg, sizeof(msg),
                         "SOEM: waiting SAFE_OP (%u) state=0x%02X AL=0x%04X",
                         phase_counter, soem_context.slavelist[1].state, sl_al);
          soem_log(msg);

          /* Re-request SAFE_OP periodically */
          soem_context.slavelist[0].state = EC_STATE_SAFE_OP;
          ecx_writestate(&soem_context, 0);
        }

        if (phase_counter > 100U)
        {
          soem_log("SOEM: SAFE_OP timeout — restarting config");
          config_phase = 0;
          phase_counter = 0;
        }
        return;
      }

      case 2: /* --- WAIT OPERATIONAL (non-blocking) --- */
      {
        (void)ecx_send_processdata(&soem_context);
        (void)ecx_receive_processdata(&soem_context, EC_TIMEOUTRET);

        ecx_readstate(&soem_context);
        uint16 sl_state = soem_context.slavelist[1].state;
        phase_counter++;

        if (sl_state == EC_STATE_OPERATIONAL)
        {
          soem_configured = 1U;
          soem_log("[SOEM] Slave reached OPERATIONAL state");
        }
        else if ((phase_counter % 10U) == 0U)
        {
          char msg[96];
          uint16 sl_al = soem_context.slavelist[1].ALstatuscode;
          (void)snprintf(msg, sizeof(msg),
                         "SOEM: waiting OP (%u) state=0x%02X AL=0x%04X",
                         phase_counter, sl_state, sl_al);
          soem_log(msg);

          soem_context.slavelist[0].state = EC_STATE_OPERATIONAL;
          ecx_writestate(&soem_context, 0);
        }

        if (phase_counter > 100U)
        {
          soem_log("SOEM: OP timeout — restarting config");
          config_phase = 0;
          phase_counter = 0;
        }
        return;
      }

      default:
        config_phase = 0;
        return;
    }
  }

  /* IOmap is in non-cacheable RAM_CMD (0x2406C000) — no cache clean needed.
   * The DMA reads directly from physical RAM for this region. */

  (void)ecx_send_processdata(&soem_context);
  {
    int wkc = ecx_receive_processdata(&soem_context, EC_TIMEOUTRET);
    static int last_wkc = -1;
    if (wkc != last_wkc)
    {
      char line[48];
      last_wkc = wkc;
      (void)snprintf(line, sizeof(line), "SOEM: wkc=%d", wkc);
      soem_log(line);
    }
  }

  if (soem_pdo_ready != 0U)
  {
    static uint32 pdo_debug_counter = 0;
    uint16 statusword = soem_txpdo->statusword;

    /* Update shadow copies for UI access */
    soem_shadow_position   = soem_txpdo->position_actual;
    soem_shadow_velocity   = soem_txpdo->velocity_actual;
    soem_shadow_torque     = soem_txpdo->torque_actual;
    soem_shadow_statusword = statusword;
    soem_shadow_pdo_ready  = 1U;
    if (statusword != soem_last_statusword)
    {
      char line[64];
      soem_last_statusword = statusword;
      (void)snprintf(line, sizeof(line), "CIA402: SW=0x%04X", statusword);
      soem_log(line);
      {
        uint8 state = soem_decode_cia402_state(statusword);
        if (state != soem_last_state)
        {
          soem_last_state = state;
          soem_log_cia402_state(state);
        }
      }
    }

    /* Debug: periodic statusword check every 5000 cycles (~5s) */
    pdo_debug_counter++;
    if ((pdo_debug_counter % 5000U) == 0U)
    {
      char line[128];
      ec_groupt *grp = soem_context.grouplist + soem_group;
      uint8 *inp = (uint8 *)grp->inputs;
      int ilen = (grp->Ibytes > 20) ? 20 : (int)grp->Ibytes;
      /* Hexdump first 20 bytes of raw TxPDO (input) data */
      int pos = snprintf(line, sizeof(line), "IN[%d]:", ilen);
      for (int i = 0; i < ilen && pos < 120; i++)
      {
        pos += snprintf(line + pos, sizeof(line) - (size_t)pos, " %02X", inp[i]);
      }
      soem_log(line);
      (void)snprintf(line, sizeof(line), "PDO: SW=0x%04X CTL=0x%04X stg=%u cnt=%lu", 
                     statusword, soem_rxpdo->controlword, 
                     (unsigned)soem_cia402_stage, (unsigned long)pdo_debug_counter);
      soem_log(line);
    }

    soem_cia402_step(statusword);
  }
}

/* ─── UI accessors (callable from TouchGFX task) ─────────────────────────── */

int32_t SOEM_GetPositionActual(void)  { return (int32_t)soem_shadow_position; }
int32_t SOEM_GetVelocityActual(void)  { return (int32_t)soem_shadow_velocity; }
int16_t SOEM_GetTorqueActual(void)    { return (int16_t)soem_shadow_torque;   }
uint16_t SOEM_GetStatusword(void)     { return (uint16_t)soem_shadow_statusword; }
uint8_t  SOEM_GetPdoReady(void)       { return (uint8_t)soem_shadow_pdo_ready; }
uint8_t  SOEM_GetRunEnable(void)      { return (uint8_t)soem_shadow_run_enable; }

void SOEM_SetRunEnable(uint8_t enable)
{
  uint8_t requested = (enable != 0U) ? 1U : 0U;
  if (soem_shadow_run_enable != requested)
  {
    soem_shadow_run_enable = requested;
    soem_log((requested != 0U) ? "CIA402: RUN request ON" : "CIA402: RUN request OFF");
  }
}

void SOEM_SetTargetPositionDelta(int32_t delta)
{
  soem_target_position += delta;
}

void SOEM_SetTargetPositionAbs(int32_t pos)
{
  soem_target_position = pos;
}

#endif
