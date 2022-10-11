#pragma once

#include <ti_core.h>

ti_span ti_iot_property_msgs_pack(ti_span payload);
ti_span ti_iot_property_msg_pack_by_id(int index, ti_span payload);
ti_span ti_iot_property_msg_pack_by_name(const char *prop, ti_span payload);

ti_span ti_iot_telemetry_msgs_pack(ti_span payload);
ti_span ti_iot_telemetry_msg_pack_by_id(int index, ti_span payload);
ti_span ti_iot_telemetry_msg_pack_by_name(const char *prop, ti_span payload);

typedef enum {
    TICOS_IOT_VAL_TYPE_BOOLEAN,
    TICOS_IOT_VAL_TYPE_INTEGER,
    TICOS_IOT_VAL_TYPE_FLOAT,
    TICOS_IOT_VAL_TYPE_STRING,
} ti_iot_val_type_t;

typedef struct {
    const char *id;
    ti_iot_val_type_t type;
    void *func;
} ti_iot_telemetry_info_t;

typedef struct {
    const char *id;
    ti_iot_val_type_t type;
    void *upload_func;
    void *download_func;
} ti_iot_property_info_t;

typedef struct {
    const char *id;
    ti_iot_val_type_t type;
    void *func;
} ti_iot_command_info_t;
