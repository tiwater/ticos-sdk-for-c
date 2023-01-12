//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/nrfconnect_port/http.h"
#include "ticos/nrfconnect_port/fota.h"

#include "ticos/components.h"

#include "ticos/ports/ncs/version.h"
#include "ticos/ports/zephyr/root_cert_storage.h"

#include "net/download_client.h"
#include "net/fota_download.h"

#include <shell/shell.h>

//! Note: A small patch is needed to nrf in order to enable
//! as of the latest SDK release (nRF Connect SDK v1.4.x)
//! See https://ticos.io/nrf-fota for more details.
#if CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE < 400
#warning "CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE must be >= 400"

#if CONFIG_DOWNLOAD_CLIENT_STACK_SIZE < 1600
#warning "CONFIG_DOWNLOAD_CLIENT_STACK_SIZE must be >= 1600"
#endif

#error "DOWNLOAD_CLIENT_MAX_FILENAME_SIZE range may need to be extended in nrf/subsys/net/lib/download_client/Kconfig"
#endif

#if !CONFIG_TICOS_FOTA_DOWNLOAD_CALLBACK_CUSTOM
void ticos_fota_download_callback(const struct fota_download_evt *evt) {
  switch (evt->id) {
    case FOTA_DOWNLOAD_EVT_FINISHED:
      TICOS_LOG_INFO("OTA Complete, resetting to install update!");
      ticos_platform_reboot();
      break;
    default:
      break;
  }
}
#endif

int ticos_fota_start(void) {
  char *download_url = NULL;
  int rv = ticos_zephyr_port_get_download_url(&download_url);
  if (rv <= 0) {
    return rv;
  }

  TICOS_ASSERT(download_url != NULL);

  TICOS_LOG_INFO("FOTA Update Available. Starting Download!");
  rv = fota_download_init(&ticos_fota_download_callback);
  if (rv != 0) {
    TICOS_LOG_ERROR("FOTA init failed, rv=%d", rv);
    goto cleanup;
  }

  // Note: The nordic FOTA API only supports passing one root CA today. So we cycle through the
  // list of required Root CAs in use by Ticos to find the appropriate one
  const int certs[] = { kTicosRootCert_AmazonRootCa1, kTicosRootCert_DigicertRootCa,
                        kTicosRootCert_CyberTrustRoot, kTicosRootCert_DigicertRootG2 };
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(certs); i++) {

#if TICOS_NCS_VERSION_GT(1, 7)
    // In NCS 1.8 signature was changed "to accept an integer parameter specifying the PDN ID,
    // which replaces the parameter used to specify the APN"
    //
    // https://github.com/nrfconnect/sdk-nrf/blob/v1.8.0/include/net/fota_download.h#L88-L106
    rv = fota_download_start(download_url, download_url, certs[i], 0 /* pdn_id */, 0);
#else // NCS <= 1.7
    // https://github.com/nrfconnect/sdk-nrf/blob/v1.4.1/include/net/fota_download.h#L88-L106
    rv = fota_download_start(download_url, download_url, certs[i], NULL /* apn */, 0);
#endif
    if (rv == 0) {
      // success -- we are ready to start the FOTA download!
      break;
    }

    if (rv == -EALREADY) {
      TICOS_LOG_INFO("fota_download_start already in progress");
      break;
    }

    // Note: Between releases of Zephyr & NCS the error code returned for TLS handshake failures
    // has changed.
    //
    // NCS < 1.6 returned EOPNOTSUPP
    // NCS >= 1.6 returns ECONNREFUSED.
    //
    // The mapping of errno values has also changed across releases due to:
    // https://github.com/zephyrproject-rtos/zephyr/commit/165def7ea6709f7f0617591f464fb95f711d2ac0
    //
    // Therefore we print a friendly message when an _expected_ failure takes place
    // but retry regardless of error condition in case value returned changes again in the future.
    if (((errno == EOPNOTSUPP) || (errno == ECONNREFUSED)) &&
        (i != (TICOS_ARRAY_SIZE(certs) - 1))) {
      TICOS_LOG_INFO("fota_download_start likely unsuccessful due to root cert mismatch. "
                        "Trying next certificate.");
    } else {
      TICOS_LOG_ERROR("fota_download_start unexpected failure, errno=%d", (int)errno);
    }
  }
  if (rv != 0) {
    TICOS_LOG_ERROR("FOTA start failed, rv=%d", rv);
    return rv;
  }

  TICOS_LOG_INFO("FOTA In Progress");
  rv = 1;
cleanup:
  ticos_zephyr_port_release_download_url(&download_url);
  return rv;
}
