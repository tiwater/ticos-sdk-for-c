#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! A lightweight set of log utilities which can be wrapped around pre-existing logging
//! infrastructure to capture events or errors that transpired leading up to an issue.
//! See https://ticos.io/logging for detailed integration steps.
//!
//! @note These utilities are already integrated into ticos/core/debug_log.h module. If your
//! project does not have a logging subsystem, see the notes in that header about how to leverage
//! the debug_log.h module for that!
//!
//! @note The thread-safety of the module depends on ticos_lock/unlock() API. If calls can be
//! made from multiple tasks, these APIs must be implemented. Locks are _only_ held while copying
//! data into the backing circular buffer so durations will be very quick.

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"
#include "ticos/core/compact_log_compile_time_checks.h"
#include "ticos/core/compact_log_helpers.h"
#include "ticos/core/compiler.h"
#include "ticos/core/platform/debug_log.h"  // For eTicosPlatformLogLevel

#ifdef __cplusplus
extern "C" {
#endif

//! Must be called on boot to initialize the Ticos logging module
//!
//! @param buffer The ram buffer to save logs into
//! @param buf_len The length of the buffer. There's no length restriction but
//!  the more space that is available, the longer the trail of breadcrumbs that will
//!  be available upon crash
//!
//! @note Until this function is called, all other calls to the module will be no-ops
//! @return true if call was successful and false if the parameters were bad or
//!  ticos_log_boot has already been called
bool ticos_log_boot(void *buffer, size_t buf_len);

//! Change the minimum level log saved to the circular buffer
//!
//! By default, any logs >= kTicosPlatformLogLevel_Info can be saved
void ticos_log_set_min_save_level(eTicosPlatformLogLevel min_log_level);

//! Macro which can be called from a platforms pre-existing logging macro.
//!
//! For example, if your platform already has something like this
//!
//! #define YOUR_PLATFORM_LOG_ERROR(...)
//!   my_platform_log_error(__VA_ARGS__)
//!
//! the error data could be automatically recorded by making the
//! following modification
//!
//! #define YOUR_PLATFORM_LOG_ERROR(...)
//!  do {
//!    TICOS_LOG_SAVE(kTicosPlatformLogLevel_Error, __VA_ARGS__);
//!    your_platform_log_error(__VA_ARGS__)
//! } while (0)
#define TICOS_LOG_SAVE(_level, ...) ticos_log_save(_level, __VA_ARGS__)

#if TICOS_COMPACT_LOG_ENABLE

//! Same as TICOS_LOG_SAVE except logs use Ticos's "compact" log strategy which offloads
//! formatting to the Ticos cloud to reduce on device codespace and cpu consumption. See
//! https://ticos.io/compact-logs for more details.
#define TICOS_COMPACT_LOG_SAVE(level, format, ...)                   \
  do {                                                                  \
    TICOS_LOGGING_RUN_COMPILE_TIME_CHECKS(format, ## __VA_ARGS__);   \
    TICOS_LOG_FMT_ELF_SECTION_ENTRY(format, ## __VA_ARGS__);         \
    ticos_compact_log_save(level,                                    \
                              TICOS_LOG_FMT_ELF_SECTION_ENTRY_PTR,   \
                              Tcs_GET_COMPRESSED_LOG_FMT(__VA_ARGS__), \
                              ## __VA_ARGS__);                          \
  } while (0)


//! Serializes the provided compact log and saves it to backing storage
//!
//! @note: Should only be called via TICOS_COMPACT_LOG_SAVE macro
void ticos_compact_log_save(eTicosPlatformLogLevel level, uint32_t log_id,
                               uint32_t compressed_fmt, ...);

#endif /* TICOS_COMPACT_LOG_ENABLE */

//! Function which can be called to save a log after it has been formatted
//!
//! Typically a user should be able to use the TICOS_LOG_SAVE macro but if your platform does
//! not have logging macros and you are just using newlib or dlib & printf, you could make
//! the following changes to the _write dependency function:
//!
//! int _write(int fd, char *ptr, int len) {
//!   // ... other code such as printing to console ...
//!   eTicosPlatformLogLevel level =
//!       (fd == 2) ? kTicosPlatformLogLevel_Error : kTicosPlatformLogLevel_Info;
//!   ticos_log_save_preformatted(level, ptr, len);
//!   // ...
//! }
void ticos_log_save_preformatted(eTicosPlatformLogLevel level, const char *log,
                                    size_t log_len);

typedef enum {
  kTicosLogRecordType_Preformatted = 0,
  kTicosLogRecordType_Compact = 1,
  kTicosLogRecordType_NumTypes,
} eTicosLogRecordType;

typedef struct {
  // the level of the message
  eTicosPlatformLogLevel level;
  // the log returned is a binary "compact log"
  // See https://ticos.io/compact-logs for more details
  eTicosLogRecordType type;
  // the length of the msg (not including NUL character)
  uint32_t msg_len;
  // the message to print which will always be NUL terminated when a preformatted log is returned
  // (so it is always safe to call printf without copying the log into another buffer yourself)
  char msg[TICOS_LOG_MAX_LINE_SAVE_LEN + 1 /* '\0' */];
} sTicosLog;

//! Returns the oldest unread log in ticos log storage
//!
//! @param log[out] When a new log is available, populated with its info
//! @return true if a log was found, false if there were no logs to read.
//!
//! @note For some timing sensitive applications, logs may be written into RAM and later dumped out
//! over UART and/or saved to flash on a lower priority background task. The ticos_log_read()
//! API is designed to be easy to utilize in these situations. For example:
//!
//! Any task:
//!   Call TICOS_SAVE_LOG() to quickly write a log into RAM.
//!
//!   Optional: Anytime a new log is saved, ticos_log_handle_saved_callback is called by the
//!   ticos log module. A platform can choose to implement something like:
//!
//!   void ticos_log_handle_saved_callback(void) {
//!       my_rtos_schedule_log_read()
//!   }
//!
//! Task responsible for flushing logs out to slower mediums (UART, NOR/EMMC Flash, etc):
//!   // .. RTOS code to wait for log read event ..
//!   sTicosLog log = { 0 };
//!   const bool log_found = ticos_log_read(&log);
//!   if (log_found && (log.type == kTicosLogRecordType_Preformatted)) {
//!       my_platform_uart_println(log.level, log, log.msg_len);
//!   }
bool ticos_log_read(sTicosLog *log);

//! Invoked every time a new log has been saved
//!
//! @note By default this is a weak function which behaves as a no-op. Platforms which dispatch
//! console logging to a low priority thread can implement this callback to have a "hook" from
//! where a job to drain new logs can easily be scheduled
extern void ticos_log_handle_saved_callback(void);

//! Formats the provided string and saves it to backing storage
//!
//! @note: Should only be called via TICOS_LOG_SAVE macro
TICOS_PRINTF_LIKE_FUNC(2, 3)
void ticos_log_save(eTicosPlatformLogLevel level, const char *fmt, ...);

//! Formats the provided string from a variable argument list
//!
//! @note Prefer saving logs via TICOS_LOG_SAVE() when possible
TICOS_PRINTF_LIKE_FUNC(2, 0)
void ticos_vlog_save(eTicosPlatformLogLevel level, const char *fmt, va_list args);

//! Freezes the contents of the log buffer in preparation of uploading the logs to Ticos.
//!
//! Once the log buffer contents have been uploaded, the buffer is unfrozen. While the buffer is
//! frozen, logs can still be added, granted enough space is available in the buffer. If the buffer
//! is full, newly logs will get dropped. Once the buffer is unfrozen again, the oldest logs will be
//! expunged again upon writing new logs that require the space.
//! @note This function must not be called from an ISR context.
void ticos_log_trigger_collection(void);

#ifdef __cplusplus
}
#endif
