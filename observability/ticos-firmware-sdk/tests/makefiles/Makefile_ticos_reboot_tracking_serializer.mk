SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_reboot_tracking_serializer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_event_storage.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_time.c \
  $(MFLT_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_reboot_tracking_serializer.cpp

include $(CPPUTEST_MAKFILE_INFRA)
