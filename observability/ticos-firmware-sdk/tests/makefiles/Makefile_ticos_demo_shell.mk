SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/demo/src/ticos_demo_shell.c

MOCK_AND_FAKE_SRC_FILES +=

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_demo_shell.c \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
