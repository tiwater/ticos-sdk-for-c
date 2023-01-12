//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ESP32 CLI implementation for demo application

// TODO: Migrate to "driver/gptimer.h" to fix warning
#include "driver/timer.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "ticos/esp_port/version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "esp_private/esp_clk.h"
  #include "soc/timer_periph.h"
#else
  #include "driver/periph_ctrl.h"
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    #include "esp32/clk.h"
  #else
    #include "esp_clk.h"
  #endif
#endif

#include "ticos/components.h"
#include "ticos/esp_port/cli.h"
#include "ticos/esp_port/http_client.h"

#define TIMER_DIVIDER  (16)  //  Hardware timer clock divider
#define TIMER_SCALE_TICKS_PER_MS(_baseFrequency)  (((_baseFrequency) / TIMER_DIVIDER) / 1000)  // convert counter value to milliseconds

static void IRAM_ATTR prv_recursive_crash(int depth) {
  if (depth == 15) {
    TICOS_ASSERT_RECORD(depth);
  }

  // an array to create some stack depth variability
  int dummy_array[depth + 1];
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(dummy_array); i++) {
    dummy_array[i] = (depth << 24) | i;
  }
  dummy_array[depth] = depth + 1;
  prv_recursive_crash(dummy_array[depth]);
}

void prv_check1(const void *buf) {
  TICOS_ASSERT_RECORD(sizeof(buf));
}

void prv_check2(const void *buf) {
  uint8_t buf2[200] = { 0 };
  prv_check1(buf2);
}

void prv_check3(const void *buf) {
  uint8_t buf3[300] = { 0 };
  prv_check2(buf3);
}

void prv_check4(void) {
  uint8_t buf4[400] = { 0 };
  prv_check3(buf4);
}

static void IRAM_ATTR prv_timer_group0_isr(void *para) {
// Always clear the interrupt:
#if CONFIG_IDF_TARGET_ESP32
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  TIMERG0.int_clr_timers.t0_int_clr = 1;
  #else
  TIMERG0.int_clr_timers.t0 = 1;
  #endif
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
  TIMERG0.int_clr_timers.t0_int_clr = 1;
#endif

  // Crash from ISR:
  ESP_ERROR_CHECK(-1);
}

static void prv_timer_init(void)
{
  const timer_config_t config = {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    .clk_src = TIMER_SRC_CLK_DEFAULT,
#endif
    .divider = TIMER_DIVIDER,
    .counter_dir = TIMER_COUNT_UP,
    .counter_en = TIMER_PAUSE,
    .alarm_en = TIMER_ALARM_EN,
    .intr_type = TIMER_INTR_LEVEL,
    .auto_reload = false,
  };
  timer_init(TIMER_GROUP_0, TIMER_0, &config);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_register(TIMER_GROUP_0, TIMER_0, prv_timer_group0_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
}

static void prv_timer_start(uint32_t timer_interval_ms) {
  uint32_t clock_hz = esp_clk_apb_freq();
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_interval_ms * TIMER_SCALE_TICKS_PER_MS(clock_hz));
  timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);
  timer_start(TIMER_GROUP_0, TIMER_0);
}

static int prv_esp32_crash_example(int argc, char** argv) {
  int crash_type =  0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  if (crash_type == 0) {
    ESP_ERROR_CHECK(10);
  } else if (crash_type == 2) {
    // Crash in timer ISR:
    prv_timer_start(10);
  } else if (crash_type == 3) {
    prv_recursive_crash(0);
  } else if (crash_type == 4) {
    prv_check4();
  }
  return 0;
}

static int prv_esp32_ticos_heartbeat_dump(int argc, char** argv) {
  ticos_metrics_heartbeat_debug_print();
  return 0;
}

static bool prv_wifi_connected_check(const char *op) {
  if (ticos_esp_port_wifi_connected()) {
    return true;
  }

  TICOS_LOG_ERROR("Must be connected to WiFi to %s. Use 'join <ssid> <pass>'", op);
  return false;
}

#if TICOS_ESP_HTTP_CLIENT_ENABLE

typedef struct {
  bool perform_ota;
} sTicosOtaUserCtx;

static bool prv_handle_ota_upload_available(void *user_ctx) {
  sTicosOtaUserCtx *ctx = (sTicosOtaUserCtx *)user_ctx;
  TICOS_LOG_DEBUG("OTA Update Available");

  if (ctx->perform_ota) {
    TICOS_LOG_INFO("Starting OTA download ...");
  }
  return ctx->perform_ota;
}

static bool prv_handle_ota_download_complete(void *user_ctx) {
  TICOS_LOG_INFO("OTA Update Complete, Rebooting System");
  esp_restart();
  return true;
}

static int prv_ticos_ota(sTicosOtaUserCtx *ctx) {
  if (!prv_wifi_connected_check("perform an OTA")) {
    return -1;
  }

  sTicosOtaUpdateHandler handler = {
    .user_ctx = ctx,
    .handle_update_available = prv_handle_ota_upload_available,
    .handle_download_complete = prv_handle_ota_download_complete,
  };

  TICOS_LOG_INFO("Checking for OTA Update");

  int rv = ticos_esp_port_ota_update(&handler);
  if (rv == 0) {
    TICOS_LOG_INFO("Up to date!");
  } else if (rv == 1) {
    TICOS_LOG_INFO("Update available!");
  } else if (rv < 0) {
    TICOS_LOG_ERROR("OTA update failed, rv=%d", rv);
  }

  return rv;
}

static int prv_ticos_ota_perform(int argc, char **argv) {
  sTicosOtaUserCtx user_ctx = {
    .perform_ota = true,
  };
  return prv_ticos_ota(&user_ctx);
}

static int prv_ticos_ota_check(int argc, char **argv) {
  sTicosOtaUserCtx user_ctx = {
    .perform_ota = false,
  };
  return prv_ticos_ota(&user_ctx);
}

static int prv_post_ticos_data(int argc, char **argv) {
  return ticos_esp_port_http_client_post_data();
}
#endif /* TICOS_ESP_HTTP_CLIENT_ENABLE */

static int prv_chunk_data_export(int argc, char **argv) {
  ticos_data_export_dump_chunks();
  return 0;
}

void ticos_register_cli(void) {
  prv_timer_init();

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "crash",
      .help = "Trigger a crash",
      .hint = "crash type",
      .func = ticos_demo_cli_cmd_crash,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "esp_crash",
      .help = "Trigger a timer isr crash",
      .hint = NULL,
      .func = prv_esp32_crash_example,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "test_log",
      .help = "Writes test logs to log buffer",
      .hint = NULL,
      .func = ticos_demo_cli_cmd_test_log,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "trigger_logs",
      .help = "Trigger capture of current log buffer contents",
      .hint = NULL,
      .func = ticos_demo_cli_cmd_trigger_logs,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "clear_core",
      .help = "Clear an existing coredump",
      .hint = NULL,
      .func = ticos_demo_cli_cmd_clear_core,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "get_core",
      .help = "Get coredump info",
      .hint = NULL,
      .func = ticos_demo_cli_cmd_get_core,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "get_device_info",
      .help = "Display device information",
      .hint = NULL,
      .func = ticos_demo_cli_cmd_get_device_info,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "export",
      .help = "Can be used to dump chunks to console or post via GDB",
      .hint = NULL,
      .func = prv_chunk_data_export,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "heartbeat_dump",
      .help = "Dump current Ticos metrics heartbeat state",
      .hint = NULL,
      .func = prv_esp32_ticos_heartbeat_dump,
  }));

#if TICOS_ESP_HTTP_CLIENT_ENABLE
  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "post_chunks",
      .help = "Post Ticos data to cloud",
      .hint = NULL,
      .func = prv_post_ticos_data,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "ticos_ota_check",
      .help = "Checks Ticos to see if a new OTA is available",
      .hint = NULL,
      .func = prv_ticos_ota_check,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "ticos_ota_perform",
      .help = "Performs an OTA is an updates is available from Ticos",
      .hint = NULL,
      .func = prv_ticos_ota_perform,
  }));
#endif /* TICOS_ESP_HTTP_CLIENT_ENABLE */
}
