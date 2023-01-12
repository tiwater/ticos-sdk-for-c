//! @file
//!
//! @brief
//! A sanity check to ensure the PEM & DER format of each cert exported match

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "ticos/core/math.h"
#include "ticos/http/root_certs.h"
#include "ticos/util/base64.h"

TEST_GROUP(TicosRootCert){
  void setup() { }
  void teardown() { }
};

static char *prv_der_to_pem(const uint8_t *der, size_t der_len) {
  const char *cert_header = "-----BEGIN CERTIFICATE-----\n";
  const char *cert_footer = "-----END CERTIFICATE-----\n";
  const size_t header_footer_len = strlen(cert_header) + strlen(cert_footer) + 1;

  // each 3 byte binary block is encoded as 4 base64 characters
  const size_t encode_len = TICOS_BASE64_ENCODE_LEN(der_len);
  // add 1 newline for each 64 characters of pem data
  const size_t linecount = encode_len / 64 + ((encode_len % 64) ? (1) : (0));
  const size_t len_with_newlines = encode_len + linecount;

  const uint32_t pem_len = len_with_newlines + header_footer_len;
  char *pem = (char *)calloc(1, pem_len);
  CHECK(pem != NULL);

  int pem_idx = sprintf(pem, "%s", cert_header);

  // encode the pem
  char *pem_body = (char *)calloc(1, encode_len);
  ticos_base64_encode(der, der_len, pem_body);

  // split into 64 characters + newline
  for (size_t i = 0; i < linecount; i++) {
    const size_t line_length = TICOS_MIN(encode_len - i * 64, 64);
    memcpy(&pem[pem_idx], &pem_body[64 * i], line_length);
    pem_idx += line_length;
    pem[pem_idx] = '\n';
    pem_idx++;
  }
  free(pem_body);

  sprintf(&pem[pem_idx], "%s", cert_footer);
  return pem;
}

//! verify the pem string has lines of max 64-characters + newline
static bool check_for_newlines(char *pemstring) {
  char *pch;
  while ((pch = strtok(pemstring, "\n")) != NULL) {
    if (strlen(pch) > 64) {
      return false;
    }

    // after first tok set to null for remaining passes
    pemstring = NULL;
  }

  return true;
}

TEST(TicosRootCert, Test_certs) {
#define FILL_CERT_ENTRY(certname, refdata) \
  { \
    .name = "" #certname "", \
    .ref_data = refdata, \
    .len = certname##_len, \
    .data = certname, \
  }

  struct cert_refs {
    const char *name;
    const char *ref_data;
    size_t len;
    const uint8_t *data;
  } cert_refs[] = {
    FILL_CERT_ENTRY(g_ticos_cert_digicert_global_root_ca,
                    TICOS_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA),
    FILL_CERT_ENTRY(g_ticos_cert_digicert_global_root_g2,
                    TICOS_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2),
    FILL_CERT_ENTRY(g_ticos_cert_baltimore_cybertrust_root,
                    TICOS_ROOT_CERTS_BALTIMORE_CYBERTRUST_ROOT),
    FILL_CERT_ENTRY(g_ticos_cert_amazon_root_ca1,
                    TICOS_ROOT_CERTS_AMAZON_ROOT_CA1),
  };

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(cert_refs); i++) {
    char *pem = prv_der_to_pem(cert_refs[i].data, cert_refs[i].len);
    STRCMP_EQUAL_TEXT(pem, cert_refs[i].ref_data, cert_refs[i].name);
    free(pem);

    char *pemstring = (char *)calloc(1, strlen(cert_refs[i].ref_data) + 1);
    strcpy(pemstring, cert_refs[i].ref_data);
    const bool newlines_correct = check_for_newlines(pemstring);
    free(pemstring);
    CHECK_TRUE_TEXT(newlines_correct, cert_refs[i].name);
  }
}
