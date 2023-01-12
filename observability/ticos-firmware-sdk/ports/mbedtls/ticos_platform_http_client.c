//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements platform dependency functions required to utilize Ticos's http client
//! for posting data collected via HTTPS with a stack making using of lwIP & Mbed TLS

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/x509.h"

#include "ticos/components.h"

//! lwIP is by far the most popular port used with Mbed TLS with embedded stacks so by default
//! include a port which implements a minimal mbedtls/net_sockets.h interface.
#ifndef TICOS_PORT_MBEDTLS_USE_LWIP
#define TICOS_PORT_MBEDTLS_USE_LWIP 1
#endif

#if TICOS_PORT_MBEDTLS_USE_LWIP

#include "lwip/debug.h"
#include "lwip/netdb.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

void mbedtls_net_init(mbedtls_net_context *ctx) {
    ctx->fd = -1;
}

int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host,
                        const char *port, int proto) {
  struct addrinfo hints = {
    .ai_family   = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE,
  };
  struct addrinfo *res = NULL;

  int ret = getaddrinfo(TICOS_HTTP_CHUNKS_API_HOST, port, &hints, &res);
  if ((ret != 0) || (res == NULL)) {
    TICOS_LOG_ERROR("Unable to resolve IP for %s - %d", TICOS_HTTP_CHUNKS_API_HOST, (int)ret);
    return ret;
  }

  ctx->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (ctx->fd < 0) {
    TICOS_LOG_ERROR("Unable to open socket");
    return -1;
  }

  ret = connect(ctx->fd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  return ret;
}

int mbedtls_net_send(void *ctx, unsigned char const *buf, size_t len) {
  mbedtls_net_context *net_ctx = (mbedtls_net_context *)ctx;
  return lwip_send(net_ctx->fd, buf, len, 0);
}


int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len ) {
  mbedtls_net_context *net_ctx = (mbedtls_net_context *)ctx;
  return lwip_recv(net_ctx->fd, (void *)buf, len, 0);
}

void mbedtls_net_close(mbedtls_net_context *ctx) {
  if (ctx->fd == -1) {
    return;
  }

  close(ctx->fd);
  ctx->fd = -1;
}

void mbedtls_net_free(mbedtls_net_context *ctx) {
  mbedtls_net_close(ctx);
}
#endif /* TICOS_PORT_MBEDTLS_USE_LWIP */

struct TcsHttpClient {
  bool active;

  mbedtls_net_context net_ctx;
  mbedtls_entropy_context entropy;
  mbedtls_hmac_drbg_context hmac_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  uint32_t flags;
  mbedtls_x509_crt cacert;
};

typedef struct TcsHttpResponse {
  uint32_t status_code;
} sTcsHttpResponse;

static sTcsHttpClient s_client;

static void prv_teardown_mbedtls(sTcsHttpClient *client) {
  mbedtls_net_free(&client->net_ctx);
  mbedtls_x509_crt_free(&client->cacert);
  mbedtls_ssl_free(&client->ssl);
  mbedtls_ssl_config_free(&client->conf);
  mbedtls_hmac_drbg_free(&client->hmac_drbg);
  mbedtls_entropy_free(&client->entropy);
}

sTcsHttpClient *ticos_platform_http_client_create(void) {
  if (s_client.active) {
    TICOS_LOG_ERROR("Ticos HTTP client already in use");
    return NULL;
  }

  mbedtls_ssl_init(&s_client.ssl);
  mbedtls_ssl_config_init(&s_client.conf);
  mbedtls_hmac_drbg_init(&s_client.hmac_drbg);
  mbedtls_x509_crt_init(&s_client.cacert);
  mbedtls_net_init(&s_client.net_ctx);

  mbedtls_entropy_init(&s_client.entropy);
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

  #define HMAC_PERS "ticos"
  int ret = mbedtls_hmac_drbg_seed(&s_client.hmac_drbg, md_info, mbedtls_entropy_func,
                                   &s_client.entropy, (const unsigned char *)HMAC_PERS,
                                   sizeof(HMAC_PERS) - 1);
  if (ret != 0) {
    TICOS_LOG_ERROR("mbedtls_hmac_drbg_seed returned -0x%x", -ret);
    goto cleanup;
  }

  ret = mbedtls_x509_crt_parse(&s_client.cacert, (const unsigned char *)TICOS_ROOT_CERTS_PEM, sizeof(TICOS_ROOT_CERTS_PEM));
  if (ret != 0) {
    TICOS_LOG_ERROR("mbedtls_x509_crt_parse returned -0x%x", -ret);
    goto cleanup;
  }

  // Perform DNS lookup for Ticos server and open connection
  char port[10] = { 0 };
  snprintf(port, sizeof(port), "%d", TICOS_HTTP_GET_CHUNKS_API_PORT());
  ret = mbedtls_net_connect(&s_client.net_ctx, TICOS_HTTP_CHUNKS_API_HOST, port, MBEDTLS_NET_PROTO_TCP);
  if (ret != 0) {
    TICOS_LOG_ERROR("mbedtls_net_connect returned -0x%x", -ret);
  }
  ret = mbedtls_ssl_config_defaults(&s_client.conf,
                                    MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    TICOS_LOG_ERROR("mbedtls_ssl_config_defaults returned -0x%x", -ret);
    goto cleanup;
  }

  mbedtls_ssl_conf_authmode(&s_client.conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&s_client.conf, &s_client.cacert, NULL);
  mbedtls_ssl_conf_rng(&s_client.conf, mbedtls_hmac_drbg_random, &s_client.hmac_drbg);

  ret = mbedtls_ssl_setup(&s_client.ssl, &s_client.conf);
  if (ret != 0) {
    TICOS_LOG_ERROR("mbedtls_ssl_setup returned -0x%x", -ret);
  }

  ret = mbedtls_ssl_set_hostname(&s_client.ssl, TICOS_HTTP_CHUNKS_API_HOST);
  if (ret) {
    TICOS_LOG_ERROR("mbedtls_ssl_set_hostname returned -0x%x", -ret);
    goto cleanup;
  }

  mbedtls_ssl_set_bio(&s_client.ssl, &s_client.net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL );

  do {
    ret = mbedtls_ssl_handshake(&s_client.ssl);
    if (ret == 0) {
      break;
    }

    if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
      continue;
    }
    TICOS_LOG_ERROR("mbedtls_ssl_handshake returned -0x%x\n", -ret);
  } while (1);


  s_client.active = true;
  return &s_client;

cleanup:
  prv_teardown_mbedtls(&s_client);
  return NULL;
}

int ticos_platform_http_client_destroy(sTcsHttpClient *client) {
  if (!client->active) {
    return -1;
  }

  // teardown the connection
  prv_teardown_mbedtls(client);
  s_client.active = false;
  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(
    TICOS_UNUSED sTcsHttpClient *client, TICOS_UNUSED uint32_t timeout_ms) {
  // No-op because ticos_platform_http_client_post_data() is synchronous
  return 0;
}

static bool prv_try_send(sTcsHttpClient *client, const uint8_t *buf, size_t buf_len) {
  size_t idx = 0;
  while (idx != buf_len) {
    int rv = mbedtls_ssl_write(&client->ssl, (const unsigned char *)buf, buf_len);
    if (rv >= 0) {
      idx += rv;
      continue;
    }

    TICOS_LOG_ERROR("Data Send Error: bytes_sent=%d, ret=0x%x", (int)idx, rv);
    return false;
  }
  return true;
}

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  sTcsHttpClient *client = (sTcsHttpClient *)ctx;
  return prv_try_send(client, data, data_len);
}

static bool prv_read_socket_data(sTcsHttpClient *client, void *buf, size_t *buf_len) {
  int rv = mbedtls_ssl_read(&client->ssl, buf, *buf_len);
  if ((rv == MBEDTLS_ERR_SSL_WANT_READ) || (rv == MBEDTLS_ERR_SSL_WANT_WRITE)) {
    *buf_len = 0;
    return true;
  }

  if ((rv < 0) || (rv == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)) {
    return false;
  }

  *buf_len = rv;
  return true;
}

static int prv_wait_for_http_response(sTcsHttpClient *client) {
  sTicosHttpResponseContext ctx = { 0 };
  while (1) {
    // We don't expect any response that needs to be parsed so
    // just use an arbitrarily small receive buffer
    char buf[32];
    size_t bytes_read = sizeof(buf);
    if (!prv_read_socket_data(client, buf, &bytes_read)) {
      return -1;
    }

    bool done = ticos_http_parse_response(&ctx, buf, bytes_read);
    if (done) {
      TICOS_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!", (int)ctx.parse_error,
                         ctx.http_status_code);
      TICOS_LOG_DEBUG("Body: %s", ctx.http_body);
      return ctx.http_status_code;
    }
  }
}

int ticos_platform_http_client_post_data(
    sTcsHttpClient *client, TicosHttpClientResponseCallback callback, void *ctx) {
  if (!client->active) {
    return -1;
  }

  const sPacketizerConfig cfg = {
    // let a single msg span many "ticos_packetizer_get_next" calls
    .enable_multi_packet_chunk = true,
  };

  // will be populated with size of entire message queued for sending
  sPacketizerMetadata metadata;
  const bool data_available = ticos_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    TICOS_LOG_DEBUG("No more data to send");
    return kTcsPostDataStatus_NoDataFound;
  }

  ticos_http_start_chunk_post(prv_send_data, client, metadata.single_chunk_message_length);

  // Drain all the data that is available to be sent
  while (1) {
    // Arbitrarily sized send buffer.
    uint8_t buf[128];
    size_t buf_len = sizeof(buf);
    eTicosPacketizerStatus status = ticos_packetizer_get_next(buf, &buf_len);
    if (status == kTicosPacketizerStatus_NoMoreData) {
      break;
    }

    if (!prv_try_send(client, buf, buf_len)) {
      // unexpected failure, abort in-flight transaction
      ticos_packetizer_abort();
      return false;
    }

    if (status == kTicosPacketizerStatus_EndOfChunk) {
      break;
    }
  }

  // we've sent a chunk, drain status
  sTcsHttpResponse response = {
    .status_code = prv_wait_for_http_response(client),
  };

  if (callback) {
    callback(&response, ctx);
  }

  return response.status_code != 202 ? response.status_code : 0;
}

int ticos_platform_http_response_get_status(const sTcsHttpResponse *response, uint32_t *status_out) {
  TICOS_SDK_ASSERT(response != NULL);

  *status_out = response->status_code;
  return 0;
}
