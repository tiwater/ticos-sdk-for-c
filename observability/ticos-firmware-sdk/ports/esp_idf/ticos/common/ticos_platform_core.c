//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <stdlib.h>
#include <stdarg.h>

#include "ticos/components.h"

#include "ticos/esp_port/cli.h"
#include "ticos/esp_port/version.h"
#include "ticos/metrics/metrics.h"

#ifndef ESP_PLATFORM
#  error "The port assumes the esp-idf is in use!"
#endif

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_cpu.h"
#else
#include "soc/cpu.h"
#endif
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"

static uint8_t s_event_storage[CONFIG_TICOS_EVENT_STORAGE_RAM_SIZE];
static uint8_t s_log_buf_storage[CONFIG_TICOS_LOG_STORAGE_RAM_SIZE];

// The default '.noninit' section is placed at the end of DRAM, which can easily
// move abosolute position if there's any change in the size of that section (eg
// through OTA update). To better preserve the absolute position of the reboot
// tracking data, we place it in the RTC noinit section '.rtc_noinit'
static RTC_NOINIT_ATTR uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

TICOS_WEAK
bool ticos_esp_port_data_available(void) {
  return ticos_packetizer_data_available();
}

TICOS_WEAK
bool ticos_esp_port_get_chunk(void *buf, size_t *buf_len) {
  return ticos_packetizer_get_chunk(buf, buf_len);
}

uint64_t ticos_platform_get_time_since_boot_ms(void) {
  const int64_t time_since_boot_us = esp_timer_get_time();
  return  (uint64_t) (time_since_boot_us / 1000) /* us per ms */;
}

bool ticos_arch_is_inside_isr(void) {
  return xPortInIsrContext();
}

TICOS_NORETURN void ticos_sdk_assert_func_noreturn(void) {
  // Note: The esp-idf implements abort which will invoke the esp-idf coredump handler as well as a
  // chip reboot so we just piggback off of that
  abort();
}

void ticos_platform_halt_if_debugging(void) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  if (esp_cpu_dbgr_is_attached()) {
#else
  if (esp_cpu_in_ocd_debug_mode()) {
#endif
    TICOS_BREAKPOINT();
  }
}

static void prv_record_reboot_reason(void) {
  eTicosRebootReason reboot_reason = kTcsRebootReason_Unknown;
  int esp_reset_cause = 0;

  // esp_reset_reason is not implemented for 3.x builds
#if defined(ESP_IDF_VERSION)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  esp_reset_cause = (int)esp_reset_reason();
  switch (esp_reset_cause) {
    case ESP_RST_POWERON:
      reboot_reason = kTcsRebootReason_PowerOnReset;
      break;
    case ESP_RST_SW:
      reboot_reason = kTcsRebootReason_SoftwareReset;
      break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      reboot_reason = kTcsRebootReason_HardwareWatchdog;
      break;
    case ESP_RST_DEEPSLEEP:
      reboot_reason = kTcsRebootReason_DeepSleep;
      break;
    case ESP_RST_BROWNOUT:
      reboot_reason = kTcsRebootReason_BrownOutReset;
      break;
    case ESP_RST_PANIC:
    default:
      reboot_reason = kTcsRebootReason_UnknownError;
      break;
  }
#endif
#endif

  const sResetBootupInfo reset_info = {
    .reset_reason_reg = esp_reset_cause,
    .reset_reason = reboot_reason,
  };

  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

static SemaphoreHandle_t s_ticos_lock;

void ticos_lock(void) {
  xSemaphoreTakeRecursive(s_ticos_lock, portMAX_DELAY);
}

void ticos_unlock(void) {
  xSemaphoreGiveRecursive(s_ticos_lock);
}

// The esp32 uses vprintf() for dumping to console. The libc implementation used requires a lot of
// stack space. We therefore prevent this function from being inlined so the log_buf allocation
// does not wind up in that path.
__attribute__((noinline))
static int prv_copy_log_to_tcs_buffer(const char *fmt, va_list args) {
  // copy result into ticos log buffer collected as part of a coredump
  char log_buf[80];
  const size_t available_space = sizeof(log_buf);
  const int rv = vsnprintf(log_buf, available_space, fmt, args);

  if (rv <= 0) {
    return -1;
  }

  size_t bytes_written = (size_t)rv;
  if (bytes_written >= available_space) {
    bytes_written = available_space - 1;
  }
  ticos_log_save_preformatted(kTicosPlatformLogLevel_Info, log_buf, bytes_written);
  return rv;
}

static int prv_ticos_log_wrapper(const char *fmt, va_list args) {
  prv_copy_log_to_tcs_buffer(fmt, args);

  // flush to stdout
  return vprintf(fmt, args);
}

void ticos_boot(void) {
  s_ticos_lock = xSemaphoreCreateRecursiveMutex();

  // set up log collection so recent logs can be viewed in coredump
  ticos_log_boot(s_log_buf_storage, sizeof(s_log_buf_storage));
  esp_log_set_vprintf(&prv_ticos_log_wrapper);

  prv_record_reboot_reason();

  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);
  ticos_reboot_tracking_collect_reset_info(evt_storage);

  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  ticos_build_info_dump();

#if CONFIG_TICOS_CLI_ENABLED
  // register CLI for easily testing Ticos
  ticos_register_cli();
#endif
}

#if CONFIG_TICOS_AUTOMATIC_INIT
// register an initialization routine that will be run from do_global_ctors()
static void __attribute__((constructor)) prv_ticos_boot(void) {
  ticos_boot();
}
#endif
