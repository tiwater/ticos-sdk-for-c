//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks the Ticos logging API to zephyr's latest (V2) logging system.
//! As of Zephyr 3.2, it's the only logging system that can be used.

#include "ticos/components.h"
#include "ticos/ports/zephyr/version.h"

#include <logging/log.h>
#include <logging/log_backend.h>
#include <logging/log_output.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Deal with CONFIG_LOG_IMMEDIATE getting renamed to CONFIG_LOG_MODE_IMMEDIATE in v3.0.0 release:
//  https://github.com/zephyrproject-rtos/zephyr/commit/262cc55609b73ea61b5f999c6c6daaba20bc5240
#if defined(CONFIG_LOG_IMMEDIATE) && !defined(CONFIG_LOG_MODE_IMMEDIATE)
#define CONFIG_LOG_MODE_IMMEDIATE CONFIG_LOG_IMMEDIATE
#endif

// Can't be zero but should be reasonably sized. See ports/zephyr/Kconfig to change this size.
BUILD_ASSERT(CONFIG_TICOS_LOGGING_RAM_SIZE);

// Define a log_output_tcs_control_block and log_output_tcs that references
// log_output_tcs_control_block via pointer. First arg to LOG_OUTPUT_DEFINE()
// is a token that the C preprocessor uses to create symbol names,
// e.g. struct log_output n and struct log_output_control_block n_control_block.
// Within the log_output_control_block there is a void *ctx we can use to store
// info we need when called by Zephyr by calling log_output_ctx_set().

static int prv_log_out(uint8_t *data, size_t length, void *ctx);
static uint8_t s_zephyr_render_buf[128];
LOG_OUTPUT_DEFINE(s_log_output_tcs, prv_log_out, s_zephyr_render_buf, sizeof(s_zephyr_render_buf));

// Zephyr will call our init function so we can establish some storage.
static void prv_log_init(const struct log_backend *const backend) {
  // static RAM storage where logs will be stored. Storage can be any size
  // you want but you will want it to be able to hold at least a couple logs.
  static uint8_t s_tcs_log_buf_storage[CONFIG_TICOS_LOGGING_RAM_SIZE];
  ticos_log_boot(s_tcs_log_buf_storage, sizeof(s_tcs_log_buf_storage));
}

static void prv_log_panic(struct log_backend const *const backend) {
  log_output_flush(&s_log_output_tcs);
}

static eTicosPlatformLogLevel prv_map_zephyr_level_to_ticos(uint32_t zephyr_level) {
  //     Map             From            To
  return zephyr_level == LOG_LEVEL_ERR ? kTicosPlatformLogLevel_Error
       : zephyr_level == LOG_LEVEL_WRN ? kTicosPlatformLogLevel_Warning
       : zephyr_level == LOG_LEVEL_INF ? kTicosPlatformLogLevel_Info
       :              /* LOG_LEVEL_DBG */kTicosPlatformLogLevel_Debug;
}

typedef struct TcsLogProcessCtx {
  eTicosPlatformLogLevel ticos_level;

#if CONFIG_LOG_MODE_IMMEDIATE
  int write_idx;
#endif

} sTcsLogProcessCtx;

// LOG2 was added in Zephyr 2.6:
// https://github.com/zephyrproject-rtos/zephyr/commit/f1bb20f6b43b8b241e45f3f132f0e7bbfc65401b
// LOG2 was moved to LOG in Zephyr 3.2
// https://github.com/zephyrproject-rtos/zephyr/issues/46500
#if !TICOS_ZEPHYR_VERSION_GT(3, 1)
#define log_msg_generic log_msg2_generic
#define log_output_msg_process log_output_msg2_process
#define log_msg_get_level log_msg2_get_level
#endif

static void prv_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  // Copied flagging from Zephry's ring buffer (rb) implementation.
  const uint32_t flags = IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)
                       ? LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP | LOG_OUTPUT_FLAG_LEVEL
                       : LOG_OUTPUT_FLAG_LEVEL;

  // Note: This is going to trigger calls to our prv_log_out() handler

  const uint32_t zephyr_level = log_msg_get_level(&msg->log);

  // This only needs to stay in scope for this function since log_output_msg_process()
  // calls prv_log_out() directly which is the only place we access the context
  sTcsLogProcessCtx log_process_ctx = {
    .ticos_level = prv_map_zephyr_level_to_ticos(zephyr_level),
  };

  // Immediate mode triggers log invocations directly from the thread doing the log. Since we are
  // buffering the log in a shared buffer, s_zephyr_render_buf, we need to hold a lock while the
  // operation is in progress.
#if CONFIG_LOG_MODE_IMMEDIATE
  ticos_lock();
#endif

  log_output_ctx_set(&s_log_output_tcs, &log_process_ctx);
  log_output_msg_process(&s_log_output_tcs, &msg->log, flags);

#if CONFIG_LOG_MODE_IMMEDIATE
  ticos_unlock();
#endif
}

static void prv_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
  ARG_UNUSED(backend);
  log_output_dropped_process(&s_log_output_tcs, cnt);
}

const struct log_backend_api log_backend_tcs_api = {
  .process          = prv_log_process,
  .panic            = prv_log_panic,
  .init             = prv_log_init,
  .dropped          = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : prv_log_dropped,
};

// Define a couple of structs needed by the logging backend infrastructure.
// Binds our log_backend_tcs_api into the logger.
LOG_BACKEND_DEFINE(log_backend_tcs, log_backend_tcs_api, true);

// Tie Ticos's log function to the Zephyr buffer sender. This is *the* connection to Ticos.
static int prv_log_out(uint8_t *data, size_t length, void *ctx) {
  if (ticos_arch_is_inside_isr()) {
    // In synchronous mode, logging can occur from ISRs. The zephyr fault handlers are chatty so
    // don't save info while in an ISR to avoid wrapping over the info we are collecting.
    return (int) length;
  }

  sTcsLogProcessCtx *tcs_ctx = (sTcsLogProcessCtx*)ctx;
  size_t save_length = length;

#if CONFIG_LOG_MODE_IMMEDIATE

  // A check to help us catch if behavior changes in a future release of Zephyr
  TICOS_SDK_ASSERT(length <= 1);

  // A few notes about immediate mode:
  //
  //  1. We are going to re-purpose "s_zephyr_render_buf" as it is never actually used by the
  //     Zephyr logging stack in immediate mode and we don't want to waste extra ram:
  //       https://github.com/zephyrproject-rtos/zephyr/blob/15fdee04e3daf4d63064e4195aeeef6ccc52e694/subsys/logging/log_output.c#L105-L112
  //
  //  2. We will use length == 0 to determine that log has been entirely
  //     flushed. log_output_msg_process() always calls log_output_flush() but in immediate mode
  //     there is no buffer filled to be flushed so the length will be zero
  if (length > 0 && tcs_ctx->write_idx < sizeof(s_zephyr_render_buf)) {
      s_zephyr_render_buf[tcs_ctx->write_idx] = data[0];
      tcs_ctx->write_idx++;
  }

  const bool flush = (length == 0);
  if (!flush) {
    return (int) length;
  }

  // Strip EOL characters at end of log since we are storing _lines_
  save_length = tcs_ctx->write_idx;
  while ((save_length - 1) > 0) {
    char c = s_zephyr_render_buf[save_length - 1];
    if ((c == '\r') || (c == '\n')) {
      save_length--;
      continue;
    }
    break;
  }
  data = &s_zephyr_render_buf[0];
#endif

  // Note: Context should always be populated via our call to log_output_ctx_set() above.
  // Assert to catch any behavior changes in future versions of Zephyr
  TICOS_SDK_ASSERT(tcs_ctx != NULL);
  ticos_log_save_preformatted(tcs_ctx->ticos_level, data, save_length);
  return (int) length;
}
