//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Installs root certificates using Zephyrs default infrastructure

#include "ticos/ports/zephyr/root_cert_storage.h"

#include <net/tls_credentials.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"

#define TICOS_NUM_CERTS_REGISTERED 4

#if !defined(CONFIG_TLS_MAX_CREDENTIALS_NUMBER) || (CONFIG_TLS_MAX_CREDENTIALS_NUMBER < TICOS_NUM_CERTS_REGISTERED)
# pragma message("ERROR: CONFIG_TLS_MAX_CREDENTIALS_NUMBER must be >= "TICOS_EXPAND_AND_QUOTE(TICOS_NUM_CERTS_REGISTERED))
# error "Update CONFIG_TLS_MAX_CREDENTIALS_NUMBER in prj.conf"
#endif

#if !defined(CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS) || (CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS < TICOS_NUM_CERTS_REGISTERED)
# pragma message("ERROR: CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS must be >= "TICOS_EXPAND_AND_QUOTE(TICOS_NUM_CERTS_REGISTERED))
# error "Update CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS in prj.conf"
#endif

TICOS_STATIC_ASSERT(
    (kTicosRootCert_MaxIndex - (kTicosRootCert_Base + 1)) == TICOS_NUM_CERTS_REGISTERED,
    "TICOS_NUM_CERTS_REGISTERED define out of sync");

int ticos_root_cert_storage_add(
    eTicosRootCert cert_id, const char *cert, size_t cert_length) {
  return tls_credential_add(cert_id, TLS_CREDENTIAL_CA_CERTIFICATE, cert, cert_length);
}
