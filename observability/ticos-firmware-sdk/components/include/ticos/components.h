#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A convenience header to pick up all the possible Ticos SDK APIs available
//!
//! For example, in a platform port file, a user of the SDK can pick up this single header and
//! start implementing dependencies without needed any other ticos component headers:
//!
//!  #include ticos/components.h
//!

#ifdef __cplusplus
extern "C" {
#endif

#include "ticos/config.h"
#include "ticos/core/arch.h"
#include "ticos/core/batched_events.h"
#include "ticos/core/build_info.h"
#include "ticos/core/compiler.h"
#include "ticos/core/custom_data_recording.h"
#include "ticos/core/data_export.h"
#include "ticos/core/data_packetizer.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/device_info.h"
#include "ticos/core/errors.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/heap_stats.h"
#include "ticos/core/log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/platform/crc32.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/platform/nonvolatile_event_storage.h"
#include "ticos/core/platform/overrides.h"
#include "ticos/core/platform/reboot_tracking.h"
#include "ticos/core/platform/system_time.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/core/sdk_assert.h"
#include "ticos/core/task_watchdog.h"
#include "ticos/core/trace_event.h"
#include "ticos/demo/cli.h"
#include "ticos/demo/shell.h"
#include "ticos/demo/shell_commands.h"
#include "ticos/demo/util.h"
#include "ticos/http/http_client.h"
#include "ticos/http/platform/http_client.h"
#include "ticos/http/root_certs.h"
#include "ticos/http/utils.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/overrides.h"
#include "ticos/metrics/platform/timer.h"
#include "ticos/metrics/utils.h"
#include "ticos/panics/assert.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/fault_handling.h"
#include "ticos/panics/platform/coredump.h"
#include "ticos/util/base64.h"
#include "ticos/util/cbor.h"
#include "ticos/util/circular_buffer.h"
#include "ticos/util/crc16_ccitt.h"
#include "ticos/util/rle.h"
#include "ticos/util/varint.h"

#ifdef __cplusplus
}
#endif
