//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <functional>
#include <stdarg.h>
#include <stdio.h>

#define PARTICLE_USE_UNSTABLE_API
#include "deviceid_hal.h"
#include "system_version.h"

#include "ticos.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("ticos_lib")

#define PARTICLE_SYSTEM_VERSION_GET_MAJOR(version) (((version) >> 24) & 0xFF)
#define PARTICLE_SYSTEM_VERSION_GET_MINOR(version) (((version) >> 16) & 0xFF)

#if PARTICLE_SYSTEM_VERSION_GET_MAJOR(SYSTEM_VERSION) < 3
  #error "Ticos Library is only compatible with Device OS 3.0 or greater"
#endif

#if ((PARTICLE_SYSTEM_VERSION_GET_MAJOR(SYSTEM_VERSION) != TICOS_PARTICLE_SYSTEM_VERSION_MAJOR) || \
     (PARTICLE_SYSTEM_VERSION_GET_MINOR(SYSTEM_VERSION) != TICOS_PARTICLE_SYSTEM_VERSION_MINOR))
  #warning "Ticos Library is targetting a different Device OS version than the one configured"
#endif

using namespace std::placeholders;

// battery backed SRAM - preserved over a reboot
static retained uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

static char s_hardware_version[32] = "Unset-Hw";
static char s_system_version[32] = TICOS_EXPAND_AND_QUOTE(SYSTEM_VERSION_STRING);

#if TICOS_PARTICLE_PORT_PANIC_HANDLER_HOOK_ENABLE
static void prv_ticos_panic_handler(const ePanicCode code, const void *extraInfo) {
  TICOS_ASSERT_RECORD(code);
}
#endif

// Here we catch SDK asserts and invoke the panic handler. We do not need to worry about getting
// into reset loops from within the Ticos SDK since the panic handling code in Ticos does not
// issue any asserts
void ticos_sdk_assert_func_noreturn(void) {
  PANIC(AssertionFailure, "ticos");

  TICOS_UNREACHABLE;
}

void Ticos::process(void) {
  if (!m_connected) {
    // we are not connected, skip attempting to send data
    return;
  }

  static uint32_t s_last_check_time = 0;
  if ((millis() - s_last_check_time) < 1000) {
    return; // we just sent data, wait a little bit
  }

  s_last_check_time = millis();

  if (!ticos_packetizer_data_available()) {
    return; // no data to send!
  }

  const size_t event_data_max_size = Particle.maxEventDataSize();

  char *chunk_buf = (char *)malloc(event_data_max_size + 1 /* for '\0' */);
  if (chunk_buf == NULL) {
    TICOS_LOG_ERROR("Unable to allocate buffer for ticos-chunks, size=%d",
                       (int)event_data_max_size);
    return;
  }

  size_t chunk_bin_len = TICOS_BASE64_MAX_DECODE_LEN(event_data_max_size);

  const bool data_available = ticos_packetizer_get_chunk(chunk_buf, &chunk_bin_len);
  if (data_available) {
    const size_t base64_len = TICOS_BASE64_ENCODE_LEN(chunk_bin_len);
    TICOS_SDK_ASSERT(base64_len <= event_data_max_size);

    ticos_base64_encode_inplace(chunk_buf, chunk_bin_len);
    chunk_buf[base64_len] = '\0';

    if (!Particle.publish("ticos-chunks", (const char *)chunk_buf)) {
      TICOS_LOG_ERROR("Failed to send Ticos Chunk, length=%d", base64_len);
    }
  }

  free(chunk_buf);
}

#if TICOS_PARTICLE_PORT_DEBUG_API_ENABLE

static int prv_test_logging(int argc, char *argv[]) {
  TICOS_LOG_DEBUG("Debug log!");
  TICOS_LOG_INFO("Info log!");
  TICOS_LOG_WARN("Warning log!");
  TICOS_LOG_ERROR("Error log!");
  return 0;
}


// Triggers an immediate heartbeat capture (instead of waiting for timer
// to expire)
static int prv_test_heartbeat(int argc, char *argv[]) {
  ticos_metrics_heartbeat_debug_trigger();
  return 0;
}

static int prv_test_trace(int argc, char *argv[]) {
  TICOS_TRACE_EVENT_WITH_LOG(TicosCli_Test, "A test error trace!");
  return 0;
}

//! Trigger a user initiated reboot and confirm reason is persisted
static int prv_test_reboot(int argc, char *argv[]) {
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_UserReset, NULL);
  ticos_platform_reboot();
}

//
// Test different crash types where a coredump should be captured
//

static int prv_test_assert(int argc, char *argv[]) {
  TICOS_ASSERT(0);
  return -1; // should never get here
}

static int prv_test_ticos(int argc, char *argv[]) {
  void (*bad_func)(void) = (void (*)())0xEEEEDEAD;
  bad_func();
  return -1; // should never get here
}

static int prv_test_nmi(int argc, char *argv[]) {
  SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
  return -1; // should never get here
}

static int prv_test_hang(int argc, char *argv[]) {
  while (1) {}
  return -1; // should never get here
}

static int prv_test_particle_panic(int argc, char *argv[]) {
  panic_(InvalidCase, NULL, HAL_Delay_Microseconds);
  return -1; // should never get here
}

static int prv_test_busfault(int argc, char *argv[]) {
  uint32_t *f = (uint32_t *)0x00000000;
  *f = 0xFFEECAFE;
  return -1; // should never get here
}

static int prv_test_spark_assert(int argc, char *argv[]) {
  SPARK_ASSERT(0);
  return -1; // should never get here
}

static int prv_test_coredump_storage(int argc, char *argv[]) {
  int32_t state = HAL_disable_irq();
  {
    ticos_coredump_storage_debug_test_begin();
  }
  HAL_enable_irq(state);
  return ticos_coredump_storage_debug_test_finish() ? 0 : -1;
}

static int prv_test_device_info(int argc, char *argv[]) {
  ticos_build_info_dump();
  ticos_device_info_dump();
  return 0;
}

// Dump Ticos data collected to console
static int prv_test_export(int argc, char *argv[]) {
  ticos_data_export_dump_chunks();
  return 0;
}

typedef struct TicosTestCommand {
  const char *name;
  int (*func)(int argc, char **argv);
} sTicosTestCommand;

static const sTicosTestCommand s_ticos_test_commands[] = {
  { "coredump_storage", prv_test_coredump_storage},
  { "device_info", prv_test_device_info },
  { "export", prv_test_export },
  { "heartbeat", prv_test_heartbeat },
  { "logging", prv_test_logging },
  { "reboot", prv_test_reboot },
  { "trace", prv_test_trace },

  // Different types of crashes
  { "assert", prv_test_assert },
  { "busfault", prv_test_busfault },
  { "hang", prv_test_hang },
  { "ticos", prv_test_ticos },
  { "nmi", prv_test_nmi },
  { "panic", prv_test_particle_panic },
  { "spark_assert", prv_test_spark_assert },
};

int Ticos::run_debug_cli_command(const char *command, int argc, char **argv) {
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_ticos_test_commands); i++) {
    if (strcmp(command, s_ticos_test_commands[i].name) != 0) {
      continue;
    }
    const int rv = s_ticos_test_commands[i].func(argc, argv);
    return rv;
  }

  TICOS_LOG_ERROR("Unknown CLI Command: %s", command);
  return -1;
}

#else

int Ticos::run_debug_cli_command(const char *command, int argc, char **argv) {
  TICOS_LOG_ERROR(
      "Debug API disabled, add TICOS_PARTICLE_PORT_DEBUG_API_ENABLE 1 to ticos config");
  return -1;
}
#endif /* TICOS_PARTICLE_PORT_DEBUG_API_ENABLE */

void Ticos::handle_cloud_connectivity_event(system_event_t event, int param) {
  if (event != cloud_status) {
    TICOS_LOG_ERROR("Unexpected cloud event type: %d", event);
    return;
  }

  m_connected = (param == cloud_status_connected);

#if TICOS_PARTICLE_PORT_CLOUD_METRICS_ENABLE
  switch (param) {
    case cloud_status_disconnected:
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Cloud_ConnectingTime));
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Cloud_ConnectedTime));
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Cloud_DisconnectCount), 1);
      break;

    case cloud_status_connecting:
      ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(Cloud_ConnectingTime));
      break;

    case cloud_status_connected:
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Cloud_ConnectingTime));
      ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(Cloud_ConnectedTime));
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Cloud_ConnectCount), 1);
      break;

    case cloud_status_disconnecting:
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Cloud_ConnectingTime));
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Cloud_ConnectedTime));
      break;
    default:
      break;
    }
#endif
}

static void prv_ticos_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t s_last_mcu_reset_reason = System.resetReason();
  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  switch (s_last_mcu_reset_reason) {
    case RESET_REASON_UNKNOWN:
      reset_reason = kTcsRebootReason_Unknown;
      break;
    case RESET_REASON_PIN_RESET:
      reset_reason = kTcsRebootReason_PinReset;
      break;
    case RESET_REASON_WATCHDOG:
      reset_reason = kTcsRebootReason_HardwareWatchdog;
      break;
    case RESET_REASON_USER:
      reset_reason = kTcsRebootReason_UserReset;
      break;
    case RESET_REASON_POWER_DOWN:
    case RESET_REASON_POWER_MANAGEMENT:
      reset_reason = kTcsRebootReason_UserShutdown;
      break;
    case RESET_REASON_POWER_BROWNOUT:
      reset_reason = kTcsRebootReason_BrownOutReset;
      break;
    case RESET_REASON_UPDATE:
      reset_reason = kTcsRebootReason_FirmwareUpdate;
      break;
    case RESET_REASON_UPDATE_TIMEOUT:
      reset_reason = kTcsRebootReason_FirmwareUpdateError;
      break;
    case RESET_REASON_PANIC:
      reset_reason = kTcsRebootReason_KernelPanic;
      break;
    case RESET_REASON_SAFE_MODE:
    case RESET_REASON_DFU_MODE:
      reset_reason = kTcsRebootReason_SoftwareReset;
      break;

    default:
      reset_reason = kTcsRebootReason_Unknown;
      break;
  }

  *info = (sResetBootupInfo){
    .reset_reason_reg = s_last_mcu_reset_reason,
    .reset_reason = reset_reason,
  };
}

uint64_t ticos_platform_get_time_since_boot_ms(void) {
  return millis();
}

static TicosPlatformTimerCallback *s_callback;

static void prv_metric_timer_callback() {
  TICOS_SDK_ASSERT(s_callback != NULL);


#if TICOS_PARTICLE_PORT_HEAP_METRICS_ENABLE
  runtime_info_t info = { 0 };
  info.size = sizeof(info);
  HAL_Core_Runtime_Info(&info, NULL);

  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_TotalSize),
                                          info.total_init_heap);
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_MinBytesFree),
                                          info.total_init_heap - info.max_used_heap);
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_BytesFree), info.freeheap);
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_MaxBlockSize),
                                          info.largest_free_block_heap);
#endif

  s_callback();

}

bool ticos_platform_metrics_timer_boot(
    uint32_t period_sec, TicosPlatformTimerCallback *callback) {
  static Timer *s_ticos_timer = NULL;

  if (s_ticos_timer == NULL) {
    s_callback = callback;
    s_ticos_timer = new Timer(period_sec * 1000, prv_metric_timer_callback);
    s_ticos_timer->start();
    TICOS_SDK_ASSERT(s_ticos_timer);
  }

  return (s_ticos_timer != NULL);
}

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (sTicosDeviceInfo){
    // NB: We intentionally leave this blank. When data is posted to ticos via
    // the particle webhook integration, the "PARTICLE_DEVICE_ID"
    // will be picked up
    .device_serial = NULL,
    .software_type = TICOS_PARTICLE_PORT_SOFTWARE_TYPE,
    .software_version = s_system_version,
    .hardware_version = s_hardware_version,
  };
}

void ticos_platform_reboot(void) {
  System.reset(RESET_NO_WAIT);

  TICOS_UNREACHABLE;
}

#if TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE

// TODO: See if we get exact TCB size exposed from device os side
#define TICOS_FREERTOS_TCB_SIZE 200

static size_t prv_get_task_region(
    os_thread_dump_info_t *info, sTcsCoredumpRegion *regions, size_t num_regions, const bool tcb) {
  if ((info == NULL) || (regions == NULL) || (num_regions == 0)) {
    return 0;
  }

  size_t region_idx = 0;

  // TCB is the thread handle in the os structure
  void *tcb_address = info->thread;

  if (tcb) {
    const size_t tcb_size = ticos_platform_sanitize_address_range(
        tcb_address, TICOS_FREERTOS_TCB_SIZE);

    if (tcb_size != 0) {
      regions[region_idx] =
          TICOS_COREDUMP_MEMORY_REGION_INIT(tcb_address, tcb_size);
      region_idx++;
    }
  } else {
    void *top_of_stack = (void *)(*(uintptr_t *)tcb_address);
    const size_t stack_size = ticos_platform_sanitize_address_range(
        top_of_stack, TICOS_PLATFORM_TASK_STACK_SIZE_TO_COLLECT);
    if (stack_size != 0) {
      regions[region_idx] =
          TICOS_COREDUMP_MEMORY_REGION_INIT(top_of_stack, stack_size);
      region_idx++;
    }
  }

  return region_idx;
}

#endif /* TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE */


size_t ticos_platform_sanitize_address_range(void *start_addr,
                                                size_t desired_size) {
  // Note: This is grabbed from the linker script and makes the assumption that
  // there is only 1 contiguous RAM region. While this is true for the NRF52840,
  // it does not hold for all MCUs so this may need to be adjusted.
  extern uint32_t _ram_start[];
  extern uint32_t _ram_end[];

  const uint32_t ram_start = (uint32_t)_ram_start;
  const uint32_t ram_end = (uint32_t)_ram_end;

  if ((uint32_t)start_addr >= ram_start && (uint32_t)start_addr < ram_end) {
    return TICOS_MIN(desired_size, ram_end - (uint32_t)start_addr);
  }
  return 0;
}

const sTcsCoredumpRegion *
ticos_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                       size_t *num_regions) {
  int region_idx = 0;

  static sTcsCoredumpRegion s_coredump_regions[16];

  // first, capture the stack that was active at the time of crash
  const size_t active_stack_size_to_collect = 512;
  s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
      crash_info->stack_address,
      ticos_platform_sanitize_address_range(crash_info->stack_address,
                                               active_stack_size_to_collect));

#if TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE
  const size_t num_coredump_regions = TICOS_ARRAY_SIZE(s_coredump_regions);

  // capture the threads and stacks (if we have space!)
  // First we will try to store all the task TCBs. This way if we run out of
  // space while storing tasks we will still be able to recover the state of all
  // the threads
  os_thread_dump(
      OS_THREAD_INVALID_HANDLE,
      [](os_thread_dump_info_t *info, void *data) -> os_result_t {
        int *region_idx = (int *)data;
        *region_idx +=
            prv_get_task_region(info, &s_coredump_regions[*region_idx],
                                num_coredump_regions - *region_idx, true);
        return 0;
      },
      &region_idx);

  // Now we store the region of the stack where context is saved. This way
  // we can unwind the stacks for threads that are not actively running
  os_thread_dump(
      OS_THREAD_INVALID_HANDLE,
      [](os_thread_dump_info_t *info, void *data) -> os_result_t {
        int *region_idx = (int *)data;
        *region_idx +=
            prv_get_task_region(info, &s_coredump_regions[*region_idx],
                                num_coredump_regions - *region_idx, false);
        return 0;
      },
      &region_idx);
#endif

  *num_regions = region_idx;
  return &s_coredump_regions[0];
}


void ticos_platform_reboot_tracking_boot(void) {
  // Read reason why device reset and update Ticos tracking accordingly
  //
  // For a detailed explanation about reboot reason storage options check out the guide at:
  //    https://ticos.io/reboot-reason-storage
  sResetBootupInfo reset_reason = {0};
  prv_ticos_reboot_reason_get(&reset_reason);
  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_reason);
}

Ticos::Ticos(const uint16_t product_version, const char *build_metadata, const char *hardware_version)
    : m_connected(false) {

  System.enableFeature(FEATURE_RESET_INFO);

  // register for system events
  System.on(cloud_status, &Ticos::handle_cloud_connectivity_event, this);

  // Grab the revision of the particle board being used for a given product type.
  if (hardware_version == NULL) {
    // Use default hardware_version scheme
    uint32_t hw_version_id = 0;
    hal_get_device_hw_version(&hw_version_id, NULL);
    snprintf(s_hardware_version, sizeof(s_hardware_version), "%s-rev%d",
             TICOS_EXPAND_AND_QUOTE(PLATFORM_NAME), (int)hw_version_id);
  } else {
    snprintf(s_hardware_version, sizeof(s_hardware_version), "%s", hardware_version);
  }

  // Uniquely identifies a firmware version running on a device.
  //
  // We concatenate the application firmware version (PRODUCT_VERSION) with the Device OS (System
  // Firmware Version) to uniquely identify the software version combination on a device.
  snprintf(s_system_version, sizeof(s_system_version), "v%d.os" TICOS_EXPAND_AND_QUOTE(SYSTEM_VERSION_STRING), (int)product_version);

  // Append metadata version info to avoid the collisions where the same version maps to different build artifacts
  const char *extra_version_info = build_metadata;
  char buildid_str[6] = { 0 };
  if ((extra_version_info == NULL) && ticos_build_id_get_string(buildid_str, sizeof(buildid_str))) {
    extra_version_info = &buildid_str[0];
  }
  if (extra_version_info != NULL) {
    const size_t curr_len = strlen(s_system_version);
    snprintf(&s_system_version[curr_len], sizeof(s_system_version) - curr_len, "+%s", extra_version_info);
  }

#if TICOS_PARTICLE_PORT_LOG_STORAGE_ENABLE
  // When set up this buffer will be captured as part of a coredump
  // so one can see messages that were logged leading up to a crash
  static uint8_t s_log_storage[TICOS_PARTICLE_PORT_LOG_STORAGE_SIZE];
  ticos_log_boot(&s_log_storage, sizeof(s_log_storage));
#endif

  ticos_build_info_dump();
  ticos_device_info_dump();
  ticos_platform_reboot_tracking_boot();

  // Initialize event storage
  static uint8_t s_event_storage[TICOS_PARTICLE_PORT_EVENT_STORAGE_SIZE]; // 1/2 day
  TICOS_STATIC_ASSERT(TICOS_PARTICLE_PORT_EVENT_STORAGE_SIZE >= 100, "TICOS_PARTICLE_PORT_EVENT_STORAGE_SIZE must be >= 100");
  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);

  // Serialize reset information so it gets reported to ticos
  ticos_reboot_tracking_collect_reset_info(evt_storage);

  // Start heartbeat subsystem
  sTicosMetricBootInfo boot_info = {
      .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  // hook into all of the ISRs
#if TICOS_PARTICLE_PORT_FAULT_HANDLERS_ENABLE
  bool isrAttachOK = true;
  isrAttachOK &=
      attachInterruptDirect(HardFault_IRQn, TICOS_EXC_HANDLER_HARD_FAULT);
  isrAttachOK &= attachInterruptDirect(MemoryManagement_IRQn,
                                       TICOS_EXC_HANDLER_MEMORY_MANAGEMENT);
  isrAttachOK &=
      attachInterruptDirect(BusFault_IRQn, TICOS_EXC_HANDLER_BUS_FAULT);
  isrAttachOK &=
      attachInterruptDirect(UsageFault_IRQn, TICOS_EXC_HANDLER_USAGE_FAULT);
  isrAttachOK &=
      attachInterruptDirect(NonMaskableInt_IRQn, TICOS_EXC_HANDLER_NMI);
  SPARK_ASSERT(isrAttachOK);
#endif

#if TICOS_PARTICLE_PORT_PANIC_HANDLER_HOOK_ENABLE
  panic_set_hook(prv_ticos_panic_handler, NULL);
#endif

  TICOS_LOG_INFO("Ticos Initialized!");
}
