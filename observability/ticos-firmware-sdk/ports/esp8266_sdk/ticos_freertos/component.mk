ifdef CONFIG_TICOS

TICOS_SDK_ROOT := ../../..

COMPONENT_SRCDIRS += \
  $(TICOS_SDK_ROOT)/ports/freertos/src

COMPONENT_ADD_INCLUDEDIRS += \
  $(TICOS_SDK_ROOT)/ports/freertos/include

# By default, the ESP8266 SDK will compile all sources in a directory so
# we explicitly list the .o here because we only want to compile one file
COMPONENT_OBJS := \
  $(TICOS_SDK_ROOT)/ports/freertos/src/ticos_freertos_ram_regions.o

else

# Disable Ticos FreeRTOS Port
COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_ADD_LDFLAGS :=
COMPONENT_SRCDIRS :=

endif
