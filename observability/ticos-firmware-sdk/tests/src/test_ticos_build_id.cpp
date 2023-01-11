//! @file

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stddef.h>

#include "ticos/core/build_info.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos_build_id_private.h"

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  memset(info, 0x0, sizeof(*info));
}

TEST_GROUP(TicosBuildInfo) {
  void setup() {
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};


#if TICOS_USE_GNU_BUILD_ID

extern "C" {
  // Emulate Symbol that would be emitted by Linker
  extern uint8_t __start_gnu_build_id_start[];

  // A dump from a real note section in an ELF:

  // arm-none-eabi-objdump -s -j .note.gnu.build-id nrf52.elf
  //
  // Contents of section .note.gnu.build-id:
  // 0050 04000000 14000000 03000000 474e5500  ............GNU.
  // 0060 58faeeb0 1696afbf b4f790c6 3b77c80f  X...........;w..
  // 0070 89972989                             ..).

  uint8_t __start_gnu_build_id_start[] = {
      0x04, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
      0x47, 0x4e, 0x55, 0x00, 0x58, 0xfa, 0xee, 0xb0, 0x16, 0x96, 0xaf, 0xbf,
      0xb4, 0xf7, 0x90, 0xc6, 0x3b, 0x77, 0xc8, 0x0f, 0x89, 0x97, 0x29, 0x89
  };

  // arm-none-eabi-readelf -n nrf52.elf
  //
  // Displaying notes found in: .note.gnu.build-id
  //  Owner                Data size    Description
  //  GNU                  0x00000014	NT_GNU_BUILD_ID (unique build ID bitstring)
  //   Build ID: 58faeeb01696afbfb4f790c63b77c80f89972989

  static const uint8_t s_expected_gnu_build_id[] = {
    0x58, 0xfa, 0xee, 0xb0, 0x16, 0x96, 0xaf, 0xbf, 0xb4, 0xf7,
    0x90, 0xc6, 0x3b, 0x77, 0xc8, 0x0f, 0x89, 0x97, 0x29, 0x89
  };
  static const char *s_expected_gnu_build_id_dump =
      "GNU Build ID: 58faeeb01696afbfb4f790c63b77c80f89972989";

}

TEST(TicosBuildInfo, Test_GnuBuildId) {
  sTicosBuildInfo info = { 0 };
  const bool found = ticos_build_info_read(&info);
  CHECK(found);

  MEMCMP_EQUAL(info.build_id, &s_expected_gnu_build_id,
               sizeof(s_expected_gnu_build_id));

  mock().expectOneCall("ticos_platform_log")
      .withIntParameter("level", kTicosPlatformLogLevel_Info)
      .withStringParameter("output", s_expected_gnu_build_id_dump);
  ticos_build_info_dump();
  mock().checkExpectations();
}

#else /* Default - Ticos Generated Build Id is In Use */

TEST(TicosBuildInfo, Test_TicosCreateUniqueVersionString) {
  const uint8_t fake_ticos_build_id[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0f, 0x10, 0x11, 0x12, 0x13
  };
  memcpy(g_ticos_sdk_derived_build_id, fake_ticos_build_id,
         sizeof(fake_ticos_build_id));

  g_ticos_build_id.type = kTicosBuildIdType_TicosBuildIdSha1;

  // Test order is important.
  // 1. NULL at genesis
  const char *unique = ticos_create_unique_version_string(NULL);
  CHECK(!unique);

  // 2. Too big at genesis
  // Instead of pulling in default_config.h just use an
  // unrealistically large size.
  char too_big[1024];
  memset(too_big, 'v', sizeof(too_big) - 1);
  too_big[sizeof(too_big) - 1] = '\0';

  unique = ticos_create_unique_version_string(too_big);
  CHECK(!unique);

  // 3. Good user version
  unique = ticos_create_unique_version_string("1.2.3");
  CHECK(unique);
  STRCMP_EQUAL("1.2.3+000102", unique);

  // 4a. Immutable checks
  unique = ticos_create_unique_version_string("changed");
  CHECK(unique);
  STRCMP_EQUAL("1.2.3+000102", unique);

  // 4b. Null should degrade into ticos_get_unique_version_string()
  // after a good version has been created.
  unique = ticos_create_unique_version_string(NULL);
  CHECK(unique);
  STRCMP_EQUAL("1.2.3+000102", unique);

  // 5. Getter check (for completeness)
  unique = ticos_get_unique_version_string();
  CHECK(unique);
  STRCMP_EQUAL("1.2.3+000102", unique);
}

TEST(TicosBuildInfo, Test_TicosBuildIdGetString) {
  const uint8_t fake_ticos_build_id[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0f, 0x10, 0x11, 0x12, 0x13
  };
  memcpy(g_ticos_sdk_derived_build_id, fake_ticos_build_id,
         sizeof(fake_ticos_build_id));

  g_ticos_build_id.type = kTicosBuildIdType_TicosBuildIdSha1;

  char build_id_str[41];
  bool success = ticos_build_id_get_string(build_id_str, 1);
  CHECK(!success);

  success = ticos_build_id_get_string(build_id_str, 6);
  CHECK(success);
  STRCMP_EQUAL("00010", build_id_str);

  success = ticos_build_id_get_string(build_id_str, 7);
  CHECK(success);
  STRCMP_EQUAL("000102", build_id_str);

  success = ticos_build_id_get_string(build_id_str, sizeof(build_id_str));
  CHECK(success);
  STRCMP_EQUAL("000102030405060708090a0b0c0d0e0f10111213", build_id_str);
}

TEST(TicosBuildInfo, Test_TicosBuildId) {
  // If a user hasn't run script to patch in build id, the value should be none
  sTicosBuildInfo info = { 0 };
  bool found = ticos_build_info_read(&info);
  CHECK(!found);

  mock().expectOneCall("ticos_platform_log")
      .withIntParameter("level", kTicosPlatformLogLevel_Info)
      .withStringParameter("output", "No Build ID available");
  ticos_build_info_dump();
  mock().checkExpectations();

  const uint8_t fake_ticos_build_id[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0f, 0x10, 0x11, 0x12, 0x13
  };
  memcpy(g_ticos_sdk_derived_build_id, fake_ticos_build_id,
         sizeof(fake_ticos_build_id));

  g_ticos_build_id.type = kTicosBuildIdType_TicosBuildIdSha1;

  found = ticos_build_info_read(&info);
  CHECK(found);
  MEMCMP_EQUAL(info.build_id, fake_ticos_build_id,
               sizeof(fake_ticos_build_id));

  mock().expectOneCall("ticos_platform_log")
      .withIntParameter("level", kTicosPlatformLogLevel_Info)
      .withStringParameter("output", "Ticos Build ID: 000102030405060708090a0b0c0d0e0f10111213");
  ticos_build_info_dump();
  mock().checkExpectations();
}

#endif /* TICOS_USE_GNU_BUILD_ID */
