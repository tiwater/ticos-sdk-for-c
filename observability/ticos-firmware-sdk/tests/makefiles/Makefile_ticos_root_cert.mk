SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/http/src/ticos_root_certs_der.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_root_cert.cpp \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_base64.c

include $(CPPUTEST_MAKFILE_INFRA)
