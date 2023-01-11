//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

#include "ticos/core/build_info.h"
#include "ticos/core/device_info.h"
#include "ticos/core/platform/device_info.h"
#include "ticos_build_id_private.h"

#include <string.h>
#include <stdio.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"

static const void *prv_get_build_id_start_pointer(void) {
  switch (g_ticos_build_id.type) {
    case kTicosBuildIdType_TicosBuildIdSha1:
      return g_ticos_build_id.storage;
    case kTicosBuildIdType_GnuBuildIdSha1: {
      const sTicosElfNoteSection *elf =
          (const sTicosElfNoteSection *)g_ticos_build_id.storage;
      return &elf->namedata[elf->namesz]; // Skip over { 'G', 'N', 'U', '\0' }
    }
    case kTicosBuildIdType_None:
    default:
      break;
  }

  return NULL;
}

bool ticos_build_info_read(sTicosBuildInfo *info) {
  const void *id = prv_get_build_id_start_pointer();
  if (id == NULL) {
    return false;
  }

  memcpy(info->build_id, id, sizeof(info->build_id));
  return true;
}

static char prv_nib_to_hex_ascii(uint8_t val) {
  return val < 10 ? (char)val + '0' : (char)(val - 10) + 'a';
}

bool ticos_build_id_get_string(char *out_buf, size_t buf_len) {
  sTicosBuildInfo info;
  bool id_available = ticos_build_info_read(&info);
  if (!id_available || buf_len < 2) {
    return false;
  }

  // 2 hex characters per byte
  const size_t chars_per_byte = 2;
  const size_t max_entries = TICOS_MIN(sizeof(info.build_id) * chars_per_byte, buf_len - 1);

  for (size_t i = 0; i < max_entries; i++) {
    const uint8_t c = info.build_id[i / chars_per_byte];
    const uint8_t nibble = (i % chars_per_byte) == 0 ? (c >> 4) & 0xf : c & 0xf;
    out_buf[i] = prv_nib_to_hex_ascii(nibble);
  }
  out_buf[max_entries] = '\0';

  return true;
}

const char *ticos_create_unique_version_string(const char * const version) {
  static char s_version[TICOS_UNIQUE_VERSION_MAX_LEN] = {0};

  // Immutable once created.
  if (s_version[0]) {
    return s_version;
  }

  if (version) {
    // Add one to account for the '+' we will insert downstream.
    const size_t version_len = strlen(version) + 1;

    // Use 6 characters of the build id to make our versions unique and
    // identifiable between releases.
    const size_t build_id_chars = 6 + 1 /* '\0' */;
    if (version_len + build_id_chars <= sizeof(s_version)) {
      memcpy(s_version, version, version_len - 1);
      s_version[version_len - 1] = '+';

      const size_t build_id_num_chars =
          TICOS_MIN(build_id_chars, sizeof(s_version) - version_len - 1);

      if (!ticos_build_id_get_string(&s_version[version_len], build_id_num_chars)) {
        // Tack on something obvious to aid with debug but don't fail.
        // We know we can safely fit 6 bytes here.
        memcpy(&s_version[version_len], "no-id", sizeof("no-id"));
        TICOS_LOG_ERROR("No configured build id");
      }

      return s_version;
    }
  }

  return NULL;
}

const char *ticos_get_unique_version_string(void) {
  return ticos_create_unique_version_string(NULL);
}

void ticos_build_info_dump(void) {
  sTicosBuildInfo info;
  bool id_available = ticos_build_info_read(&info);
  if (!id_available) {
    TICOS_LOG_INFO("No Build ID available");
    return;
  }

  const bool is_gnu =
      (g_ticos_build_id.type == kTicosBuildIdType_GnuBuildIdSha1);

  char build_id_sha[41] = { 0 };
  for (size_t i = 0; i < sizeof(info.build_id); i++) {
    uint8_t c = info.build_id[i];
    size_t idx = i * 2;
    build_id_sha[idx] = prv_nib_to_hex_ascii((c >> 4) & 0xf);
    build_id_sha[idx + 1] = prv_nib_to_hex_ascii(c & 0xf);
  }

  TICOS_LOG_INFO("%s Build ID: %s", is_gnu ? "GNU" : "Ticos", build_id_sha);
}

void ticos_device_info_dump(void) {
  struct TicosDeviceInfo info = {0};
  ticos_platform_get_device_info(&info);
  TICOS_LOG_INFO("S/N: %s", info.device_serial ? info.device_serial : "<NULL>");
  TICOS_LOG_INFO("SW type: %s", info.software_type ? info.software_type : "<NULL>");
  TICOS_LOG_INFO("SW version: %s", info.software_version ? info.software_version : "<NULL>");
  TICOS_LOG_INFO("HW version: %s", info.hardware_version ? info.hardware_version : "<NULL>");
}
