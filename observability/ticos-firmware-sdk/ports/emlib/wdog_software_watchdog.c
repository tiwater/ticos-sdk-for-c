//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A Software Watchdog implementation backed by the
//! EFM/EFR WDOG peripheral & the SiLabs Gecko SDK (EMLIB)
//!
//! The implementation uses the hardware watchdog peripheral (WDOG) but
//! configures a warning interrupt to fire at TICOS_WATCHDOG_SW_TIMEOUT_SECS
//! and the hardware watchdog to reset the device at TICOS_WATCHDOG_SW_TIMEOUT_SECS * 2.
//!
//! To configure, compile the file and wire up the IRQ handler to Ticos coredump collection
//! by updating your compiler flags: -DTICOS_EXC_HANDLER_WATCHDOG=WDOG0_IRQHandler
//!
//! Implementation notes:
//! - The WDOG peripheral supports discrete timeouts. This port chooses the closest period
//!   that is less than or equal to the requested value. Valid selections for
//!   TICOS_WATCHDOG_SW_TIMEOUT_SECS range from 1s - 128s.

#include "ticos/ports/watchdog.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"

#include "em_wdog.h"
#include "em_cmu.h"

#if defined(WDOG0)
#define TICOS_WDOG WDOG0
#define TICOS_WDOG_IRQn WDOG0_IRQn

#ifndef TICOS_EXC_HANDLER_WATCHDOG
#  error "Port expects following define to be set: -DTICOS_EXC_HANDLER_WATCHDOG=WDOG0_IRQHandler"
#endif

#elif defined(WDOG)
#define TICOS_WDOG WDOG
#define TICOS_WDOG_IRQn WDOG_IRQn

#ifndef TICOS_EXC_HANDLER_WATCHDOG
#  error "Port expects following define to be set: -DTICOS_EXC_HANDLER_WATCHDOG=WDOG_IRQHandler"
#endif

#else
# error "Could not find an available WDOG to use"
#endif

#define TICOS_EM_WDOG_FREQ_HZ 1000

#define TICOS_EM_WDOG_WARNING_TIMEOUT_MS(timeout) ((timeout) / 2)

#define TICOS_EM_WDOG_PERSEL_TO_TIMEOUT_MS(persel) \
  ((1000 * (1 << (3 + persel))) / TICOS_EM_WDOG_FREQ_HZ)

#define TICOS_EM_WDOG_MAX_TIMEOUT_MS (TICOS_EM_WDOG_PERSEL_TO_TIMEOUT_MS(wdogPeriod_256k))

static void prv_build_configuration(WDOG_Init_TypeDef *cfg, uint32_t persel) {
  *cfg = (WDOG_Init_TypeDef) {
    .enable = true,
    // freeze watchdog when a debugger halts the system
    .debugRun = false,
    .em2Run = true,
    .em3Run = true,
    .em4Block = false,
#if defined(_WDOG_CTRL_SWOSCBLOCK_MASK)
    .swoscBlock = false,
#endif
    .lock = false,
#if defined(_WDOG_CTRL_CLKSEL_MASK)
    // use internal 1kHz clock for largest range
    .clkSel = wdogClkSelULFRCO,
#else
    #error "Port doesn't support WDOG Variant - cannot configure clock"
#endif
    .perSel = persel,

#if defined(_WDOG_CTRL_WARNSEL_MASK) || defined(_WDOG_CFG_WARNSEL_MASK)
    .warnSel = wdogWarnTime50pct,
#else
    #error "Port doesn't support WDOG Variant - no early warning interrupt"
#endif

#if defined(_WDOG_CTRL_WINSEL_MASK) || defined(_WDOG_CFG_WINSEL_MASK)
    .winSel = wdogIllegalWindowDisable,
#endif

#if defined(_WDOG_CTRL_WDOGRSTDIS_MASK) || defined(_WDOG_CFG_WDOGRSTDIS_MASK)
    // we want a hardware watchdog to trigger a reset
    .resetDisable = false,
#endif
  };
}

static int prv_configure_watchdog_with_timeout(uint32_t timeout_ms) {

  // NB: An interrupt can be configured to fire at 25%, 50%, or 75% of the watchdog cycle
  // configured. We'll use this interrupt as our "software watchdog" and configure it to be at
  // the 50% interval
  const size_t sw_wdog_max_ms = TICOS_EM_WDOG_WARNING_TIMEOUT_MS(TICOS_EM_WDOG_MAX_TIMEOUT_MS);
  if (timeout_ms > sw_wdog_max_ms) {
    TICOS_LOG_ERROR("Requested wdog timeout (%d) exceeds max supported (%d)",
                       (int)timeout_ms, (int)sw_wdog_max_ms);
    return -1;
  }

  uint32_t persel = wdogPeriod_9;
  while (persel <= wdogPeriod_256k) {
    const uint32_t warning_timeout_ms =
        TICOS_EM_WDOG_WARNING_TIMEOUT_MS(TICOS_EM_WDOG_PERSEL_TO_TIMEOUT_MS(persel));
    if (warning_timeout_ms > timeout_ms) {
      break;
    }
    persel++;
  }

  // choose the closest timeout without going over the desired max
  persel = (persel == 0)  ? 0 : persel - 1;

  const size_t actual_timeout_ms = TICOS_EM_WDOG_PERSEL_TO_TIMEOUT_MS(persel);
  TICOS_LOG_DEBUG("Configuring SW Watchdog. SW Timeout=%dms HW Timeout=%dms",
                     (int)TICOS_EM_WDOG_WARNING_TIMEOUT_MS(actual_timeout_ms),
                     actual_timeout_ms);

  // Low energy clock must be on to use the watchdog
  if ((CMU->HFBUSCLKEN0 & CMU_HFBUSCLKEN0_LE) == 0) {
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_LE;
  }

  if (WDOGn_IsLocked(TICOS_WDOG)) {
    TICOS_LOG_ERROR("Watchdog is locked and cannot be reconfigured");
    return -2;
  }

  // We are about to (re-)configure the peripheral so disable it
  WDOGn_Enable(TICOS_WDOG, false);
  NVIC_DisableIRQ(TICOS_WDOG_IRQn);
  NVIC_ClearPendingIRQ(TICOS_WDOG_IRQn);

  WDOG_Init_TypeDef cfg;
  prv_build_configuration(&cfg, persel);
  WDOGn_Init(TICOS_WDOG, &cfg);

  // enable the warning interrupt. This will be used to capture a coredump rather than just letting
  // the hardware watchdog immediately reboot the system
  const uint32_t warn_int_mask = 0x2; // "WARN Interrupt Enable"
  WDOGn_IntClear(TICOS_WDOG, warn_int_mask);
  WDOGn_IntEnable(TICOS_WDOG, warn_int_mask);

  // Enable the interrupt in the NVIC and set it to the highest priority.
  // This way we can even capture hangs inside ISRs!
  NVIC_SetPriority(TICOS_WDOG_IRQn, 0);
  NVIC_EnableIRQ(TICOS_WDOG_IRQn);

  // finally, with everything setup, start the watchdog!
  WDOGn_Enable(TICOS_WDOG, true);
  WDOGn_SyncWait(TICOS_WDOG);
  return 0;
}

int ticos_software_watchdog_update_timeout(uint32_t timeout_ms) {
  return prv_configure_watchdog_with_timeout(timeout_ms);
}

int ticos_software_watchdog_enable(void) {
  const uint32_t timeout_ms = TICOS_WATCHDOG_SW_TIMEOUT_SECS * 1000;
  return ticos_software_watchdog_update_timeout(timeout_ms);
}

int ticos_software_watchdog_disable(void) {
  if (WDOGn_IsLocked(TICOS_WDOG)) {
    TICOS_LOG_ERROR("Watchdog is locked and cannot be disabled");
    return -1;
  }

  WDOGn_Enable(TICOS_WDOG, false);
  return 0;
}

int ticos_software_watchdog_feed(void) {
  // NB: A clear request takes ~4ms to propagate through the peripheral.
  // Enteriung EM2 or EM3 power states while this is in progress will
  // cause the operation to be aborted. WDOGn_SyncWait() can be used to
  // block until the operation is complete!
  WDOG_Feed();
  return 0;
}
