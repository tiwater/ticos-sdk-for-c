SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/metrics/src/ticos_metrics.c \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_serializer_helper.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_build_id.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_event_storage.cpp \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_get_device_info.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_time.c \
  $(Tcs_TEST_MOCK_DIR)/mock_ticos_reboot_tracking.cpp \
  $(Tcs_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_heartbeat_metrics.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
