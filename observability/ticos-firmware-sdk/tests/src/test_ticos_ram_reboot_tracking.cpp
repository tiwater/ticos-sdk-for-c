#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

extern "C" {
  #include "ticos/core/reboot_tracking.h"
  #include "ticos_reboot_tracking_private.h"

  static uint8_t s_tcs_reboot_tracking_region[TICOS_REBOOT_TRACKING_REGION_SIZE];
}

TEST_GROUP(TcsStorageTestGroup) {
  void setup() {
    // simulate memory initializing with random pattern at boot
    memset(&s_tcs_reboot_tracking_region[0], 0xBA, sizeof(s_tcs_reboot_tracking_region));
    ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, NULL);
    ticos_reboot_tracking_reset_crash_count();
    ticos_reboot_tracking_clear_reboot_reason();
    ticos_reboot_tracking_clear_reset_info();
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(TcsStorageTestGroup, Test_BadArgs) {
  ticos_reboot_tracking_read_reset_info(NULL);
}

TEST(TcsStorageTestGroup, Test_SetAndGetCrashInfo) {
  sTcsResetReasonInfo read_info;
  bool crash_found = ticos_reboot_tracking_read_reset_info(&read_info);
  CHECK(!crash_found);

  sTcsRebootTrackingRegInfo info = { 0 };
  info.pc = 0x1;
  info.lr = 0x2;

  const eTicosRebootReason reason = kTcsRebootReason_Assert;
  ticos_reboot_tracking_mark_reset_imminent(reason, &info);
  ticos_reboot_tracking_mark_coredump_saved();

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
  };
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);

  crash_found = ticos_reboot_tracking_read_reset_info(&read_info);
  CHECK(crash_found);

  LONGS_EQUAL(reason, read_info.reason);
  LONGS_EQUAL(info.pc, read_info.pc);
  LONGS_EQUAL(info.lr, read_info.lr);
  LONGS_EQUAL(bootup_info.reset_reason_reg, read_info.reset_reason_reg0);
  LONGS_EQUAL(1, read_info.coredump_saved);

  ticos_reboot_tracking_clear_reset_info();
  crash_found = ticos_reboot_tracking_read_reset_info(&read_info);
  CHECK(!crash_found);
}

TEST(TcsStorageTestGroup, Test_NoRamRegionIntialized) {
  ticos_reboot_tracking_boot(NULL, NULL);
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_Assert, NULL);
  sTcsResetReasonInfo info;
  bool info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
  ticos_reboot_tracking_reset_crash_count();
  ticos_reboot_tracking_get_crash_count();
  ticos_reboot_tracking_clear_reset_info();
}

TEST(TcsStorageTestGroup, Test_CrashTracking) {
  // Mark next bootup as created by an error
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_UnknownError, NULL);

  // Crash count should not be incremented yet
  size_t crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);

  // mcu reset reason being appended without a pre-existing reset reason should bump crash count
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
    .reset_reason = kTcsRebootReason_HardFault,
  };
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);

  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  // Resetting reboot tracking should not alter crash count
  ticos_reboot_tracking_clear_reset_info();
  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  ticos_reboot_tracking_reset_crash_count();
  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);
}

TEST(TcsStorageTestGroup, Test_RebootSequence) {
  size_t crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);
  // 1. Watchdog Reboot - Confirm we get a Watchdog and then no more info
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_SoftwareWatchdog, NULL);
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, NULL);

  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  sTcsResetReasonInfo info;
  bool info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kTcsRebootReason_SoftwareWatchdog, info.reason);
  ticos_reboot_tracking_clear_reset_info();
  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
  // 2. Unexpected Reboot - Info should be taken from what we provide on bootup info
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kTcsRebootReason_Assert,
  };
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);

  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(2, crash_count);
  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(bootup_info.reset_reason, info.reason);
  LONGS_EQUAL(bootup_info.reset_reason_reg, info.reset_reason_reg0);
  ticos_reboot_tracking_clear_reset_info();

  // 2 Firmware Update Reboot reboot - Reset region reg should be amended to info
  // but reset reason should not overwrite value
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_FirmwareUpdate, NULL);
  bootup_info.reset_reason_reg = 0xdead;

  // The kTcsRebootReason_Assert passed as part of bootup_info should be counted as a crash
  // but the reset reason should be the reboot which first kicked off the sequence of events
  // (kTcsRebootReason_FirmwareUpdate)
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);
  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kTcsRebootReason_FirmwareUpdate, info.reason);
  LONGS_EQUAL(bootup_info.reset_reason_reg, info.reset_reason_reg0);
  ticos_reboot_tracking_clear_reset_info();

  // 4. Unexpected Reboot (i.e POR) - Should see unknown as reset reason
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, NULL);
  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kTcsRebootReason_Unknown, info.reason);
  ticos_reboot_tracking_clear_reset_info();

  // 5. Expected reboot due to firmware update, boot with an expected reboot reason
  bootup_info.reset_reason = kTcsRebootReason_SoftwareReset;
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_FirmwareUpdate, NULL);
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);

  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kTcsRebootReason_FirmwareUpdate, info.reason);
  ticos_reboot_tracking_clear_reset_info();

  // 6. Boot with an expected reboot reason
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &bootup_info);
  info_available = ticos_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kTcsRebootReason_SoftwareReset, info.reason);

  // Scenarios 5 and 6 should not count as unexpected crashes
  crash_count = ticos_reboot_tracking_get_crash_count();
  LONGS_EQUAL(4, crash_count);
}

TEST(TcsStorageTestGroup, Test_GetRebootReason) {
  sTcsRebootReason reboot_reason = {
    .reboot_reg_reason = kTcsRebootReason_UnknownError,
  };
  int rc = 0;

  // Test that reason is invalid if arg is NULL
  rc = ticos_reboot_tracking_get_reboot_reason(NULL);

  LONGS_EQUAL(-1, rc);

  // Test that reason is invalid before reboot tracking has booted
  rc = ticos_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(-1, rc);
  LONGS_EQUAL((long)kTcsRebootReason_UnknownError, (long)reboot_reason.reboot_reg_reason);

  // Test booting with null
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, NULL);

  rc = ticos_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(0, rc);
  LONGS_EQUAL((long)kTcsRebootReason_Unknown, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kTcsRebootReason_Unknown, (long)reboot_reason.prior_stored_reason);

  // Test booting with a non-error reason, check that reboot reason is not cleared by
  // ticos_reboot_tracking_clear_reset_info
  ticos_reboot_tracking_clear_reset_info();
  ticos_reboot_tracking_clear_reboot_reason();

  sResetBootupInfo info = {
    .reset_reason = kTcsRebootReason_SoftwareReset,
  };

  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kTcsRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kTcsRebootReason_SoftwareReset, (long)reboot_reason.prior_stored_reason);

  ticos_reboot_tracking_clear_reset_info();
  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kTcsRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kTcsRebootReason_SoftwareReset, (long)reboot_reason.prior_stored_reason);

  // Simulate a crash, first boot with a crash. This will store the crash reason in reboot tracking
  // and reboot reason memory. Check values
  // Next clear reboot reason memory, and boot again with a different reason. Check values
  info.reset_reason = kTcsRebootReason_Assert;
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kTcsRebootReason_Assert, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kTcsRebootReason_Assert, (long)reboot_reason.prior_stored_reason);

  ticos_reboot_tracking_clear_reboot_reason();
  info.reset_reason = kTcsRebootReason_SoftwareReset;
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kTcsRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kTcsRebootReason_Assert, (long)reboot_reason.prior_stored_reason);
}

TEST(TcsStorageTestGroup, Test_GetUnexpectedRebootOccurred) {
  bool unexpected_reboot = false;
  int rc = 0;
  sResetBootupInfo info = {.reset_reason = kTcsRebootReason_SoftwareReset};

  // Test invalid if arg is NULL
  rc = ticos_reboot_tracking_get_unexpected_reboot_occurred(NULL);
  LONGS_EQUAL(-1, rc);

  // Test invalid before reboot tracking has booted
  rc = ticos_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(-1, rc);

  // Test booting with an expected reason
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK_FALSE(unexpected_reboot);
  ticos_reboot_tracking_clear_reset_info();

  // Test booting with an unexpected reason
  info.reset_reason = kTcsRebootReason_UnknownError;
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK(unexpected_reboot);
  ticos_reboot_tracking_clear_reset_info();

  // Test booting with an unexpected reason marked, but expected reason at boot
  info.reset_reason = kTcsRebootReason_PowerOnReset;
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_Assert, NULL);
  ticos_reboot_tracking_boot(s_tcs_reboot_tracking_region, &info);
  rc = ticos_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK(unexpected_reboot);
}
