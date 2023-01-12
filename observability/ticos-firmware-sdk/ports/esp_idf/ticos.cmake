# Typically, the platform specific dependencies (e.g. ticos_platform_get_device_info()) that are
# not part of the esp_idf default port are added to the "main" component. However, the user
# of the port may also explicitly setting the TICOS_PLATFORM_PORT_COMPONENTS prior to including this
# cmake helper:
#
# set(TICOS_PLATFORM_PORT_COMPONENTS <component>)

if(NOT DEFINED TICOS_PLATFORM_PORT_COMPONENTS)
set(TICOS_PLATFORM_PORT_COMPONENTS main)
message(STATUS "TICOS_PLATFORM_PORT_COMPONENTS not provided, using default ('${TICOS_PLATFORM_PORT_COMPONENTS}')")
endif()

# Some esp32 ports (i.e amazon-freertos) do not make use of the esp_http_client component so we
# expose a way to disable it entirely
if(NOT DEFINED TICOS_ESP_HTTP_CLIENT_ENABLE)
  set(TICOS_ESP_HTTP_CLIENT_ENABLE 1)
endif()

# Note: We want to forward some settings to the "ticos" idf component but that takes
# place at a later stage in the build process not included directly in this path so we
# pass settings using environment variables
set(ENV{TICOS_PLATFORM_PORT_COMPONENTS} ${TICOS_PLATFORM_PORT_COMPONENTS})
set(ENV{TICOS_ESP_HTTP_CLIENT_ENABLE} ${TICOS_ESP_HTTP_CLIENT_ENABLE})
set(ENV{TICOS_PLATFORM_EXTRA_INCLUDES} ${TICOS_PLATFORM_EXTRA_INCLUDES})

# This will inform esp-idf to pick up the ticos component which includes the
# ticos-firmware-sdk as well as the esp-idf porting layer that is not application specific
set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} ${CMAKE_CURRENT_LIST_DIR})
