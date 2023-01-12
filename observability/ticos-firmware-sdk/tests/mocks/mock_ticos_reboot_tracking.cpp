//! @file
//!
//! Mock for use testing components which depend on reboot tracking

#include "CppUTestExt/MockSupport.h"
#include "ticos/core/reboot_tracking.h"

int ticos_reboot_tracking_get_reboot_reason(sTcsRebootReason *reboot_reason) {
  return mock()
    .actualCall(__func__)
    .withOutputParameter("reboot_reason", reboot_reason)
    .returnIntValue();
}

int ticos_reboot_tracking_get_unexpected_reboot_occurred(bool *unexpected_reboot_occurred) {
  return mock()
    .actualCall(__func__)
    .withOutputParameter("unexpected_reboot_occurred", unexpected_reboot_occurred)
    .returnIntValue();
}
