set(srcs
        src/ticos_core.c
        src/ticos_thingmodel_op.c
        hal/esp32/ticos_mqtt_wrapper.c)

set(includes src)

idf_component_register(SRCS "${srcs}"
        INCLUDE_DIRS "${includes}"
        PRIV_INCLUDE_DIRS "${priv_includes}"
        REQUIRES json mqtt)
