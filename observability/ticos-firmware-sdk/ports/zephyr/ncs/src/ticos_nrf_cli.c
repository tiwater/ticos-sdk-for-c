//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <shell/shell.h>

#include "ticos/core/debug_log.h"
#include "ticos/nrfconnect_port/fota.h"
#include "ticos/ports/zephyr/http.h"

static int prv_tcs_fota(const struct shell *shell, size_t argc, char **argv) {
#if CONFIG_TICOS_FOTA_CLI_CMD
  TICOS_LOG_INFO("Checking for FOTA");
  const int rv = ticos_fota_start();
  if (rv == 0) {
    TICOS_LOG_INFO("FW is up to date!");
  }
  return rv;
#else
  shell_print(shell, "CONFIG_TICOS_FOTA_CLI_CMD not enabled");
  return -1;
#endif
}

#if CONFIG_TICOS_HTTP_ENABLE
static int prv_tcs_get_latest_url(const struct shell *shell, size_t argc, char **argv) {
  char *url = NULL;
  int rv = ticos_zephyr_port_get_download_url(&url);
  if (rv <= 0) {
    TICOS_LOG_ERROR("Unable to fetch OTA url, rv=%d", rv);
    return rv;
  }

  printk("Download URL: '%s'\n", url);

  rv = ticos_zephyr_port_release_download_url(&url);

  return rv;
}
#endif // CONFIG_TICOS_HTTP_ENABLE

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_ticos_nrf_cmds,
    SHELL_CMD(fota, NULL, "Perform a FOTA using Ticos client",
              prv_tcs_fota),
#if CONFIG_TICOS_HTTP_ENABLE
    SHELL_CMD(get_latest_url, NULL, "Get the latest URL for the latest FOTA",
              prv_tcs_get_latest_url),
#endif
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(tcs_nrf, &sub_ticos_nrf_cmds,
                   "Ticos nRF Connect SDK Test Commands", NULL);
