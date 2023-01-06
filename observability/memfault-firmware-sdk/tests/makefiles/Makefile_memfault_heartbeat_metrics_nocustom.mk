SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/metrics/src/memfault_metrics.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_event_storage.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_time.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_reboot_tracking.cpp \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_heartbeat_metrics_nocustom.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

# Set flag to compile with only built-in memfault metrics, to confirm
# compilation is OK
CPPUTEST_CPPFLAGS += -DTEST_NO_CUSTOM_METRICS=1

include $(CPPUTEST_MAKFILE_INFRA)
