file(GLOB core_srcs "src/*.c")
file(GLOB hal_esp32_srcs "hal/esp32/*.c" "hal/esp32/provisioning/*.c" "hal/esp32/utils/*.c")
file(GLOB user_srcs "../../main/ticos_cloud/*.c")
set(includes hal/esp32/inc src ../../main/ticos_cloud)

idf_component_register(SRCS "${core_srcs}" "${hal_esp32_srcs}" "${user_srcs}" 
        INCLUDE_DIRS "${includes}"
        PRIV_INCLUDE_DIRS "${priv_includes}"
        REQUIRES json mqtt esp_https_ota app_update wifi_provisioning  fatfs)
