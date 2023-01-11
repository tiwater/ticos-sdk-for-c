SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_log.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_circular_buffer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_crc16_ccitt.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_locking.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_log.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_COMPACT_LOG_ENABLE=1

include $(CPPUTEST_MAKFILE_INFRA)
