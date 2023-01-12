//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! Reference implementation of the ticos platform header for the WICED platform

#include "platform_assert.h" // import WICED_TRIGGER_BREAKPOINT
#include "platform_cmsis.h" // import NVIC_SystemReset & CoreDebug

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/platform/core.h"

#include "ticos/http/root_certs.h"
#include "ticos_platform_wiced.h"

#include "wiced_tls.h"
#include "wiced_result.h"

// return different error codes from each exit point so it's easier to determine what went wrong
typedef enum {
  kTicosPlatformBoot_DeviceInfoBootFailed = -1,
  kTicosPlatformBoot_CoredumpBootFailed = -2,
} eTicosPlatformBoot;

int ticos_platform_boot(void) {
  if (!ticos_platform_device_info_boot()) {
    TICOS_LOG_ERROR("ticos_platform_device_info_boot() failed");
    return kTicosPlatformBoot_DeviceInfoBootFailed;
  }
  if (!ticos_platform_coredump_boot()) {
    TICOS_LOG_ERROR("ticos_platform_coredump_boot() failed");
    return kTicosPlatformBoot_CoredumpBootFailed;
  }

  const wiced_result_t result = wiced_tls_init_root_ca_certificates(
      TICOS_ROOT_CERTS_PEM, sizeof(TICOS_ROOT_CERTS_PEM) - 1);
  if (result != WICED_SUCCESS) {
    TICOS_LOG_ERROR("wiced_tls_init_root_ca_certificates() failed: %u", result);
    return TICOS_PLATFORM_SPECIFIC_ERROR(result);
  }

  return 0;
}

TICOS_NORETURN void ticos_platform_reboot(void) {
  ticos_platform_halt_if_debugging();

  NVIC_SystemReset();
  TICOS_UNREACHABLE;
}
