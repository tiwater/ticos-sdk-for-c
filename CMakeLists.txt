set(srcs
        src/ticos_core.c
        src/ticos_thingmodel_op.c
        examples/t3/ticos_thingmodel.c
        hal/esp32/ticos_mqtt_wrapper.c)

set(includes src examples/t3 ../esp-qcloud/include )

idf_component_register(SRCS "${srcs}"
        INCLUDE_DIRS "${includes}"
        PRIV_INCLUDE_DIRS "${priv_includes}"
        REQUIRES json mqtt esp_https_ota app_update)
