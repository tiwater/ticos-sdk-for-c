ifdef CONFIG_TICOS

TICOS_SDK_ROOT := ../../..

COMPONENT_SRCDIRS += \
   $(TICOS_SDK_ROOT)/components/core/src \
   $(TICOS_SDK_ROOT)/components/demo/src \
   $(TICOS_SDK_ROOT)/components/http/src \
   $(TICOS_SDK_ROOT)/components/metrics/src \
   $(TICOS_SDK_ROOT)/components/panics/src \
   $(TICOS_SDK_ROOT)/components/util/src


COMPONENT_ADD_INCLUDEDIRS += \
  $(TICOS_SDK_ROOT)/components/include \
  $(TICOS_SDK_ROOT)/ports/esp8266_sdk/ticos/include \
  $(TICOS_SDK_ROOT)/ports/freertos/include \
  $(TICOS_SDK_ROOT)/ports/include \
  config

# Notes:
#  1. We --wrap the panicHandler so we can install a coredump collection routine
#  2. We add the .o for the RLE encoder directly because components get built as
#     a static library and we want to guarantee the weak functions get loaded
COMPONENT_ADD_LDFLAGS +=					\
  -Wl,--wrap=panicHandler					\
  build/ticos/components/core/src/ticos_data_source_rle.o

CFLAGS += \
  -DTICOS_EVENT_STORAGE_READ_BATCHING_ENABLED=1 \
  -DTICOS_METRICS_USER_HEARTBEAT_DEFS_FILE=\"ticos_metrics_heartbeat_esp_port_config.def\"

else

# Disable Ticos support
COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_ADD_LDFLAGS :=
COMPONENT_SRCDIRS :=

endif # CONFIG_TICOS
