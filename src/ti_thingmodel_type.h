#pragma once

#include <_ti_cfg_prefix.h>

#include <time.h>

typedef enum {
    TICOS_IOT_VAL_TYPE_BOOLEAN,
    TICOS_IOT_VAL_TYPE_INTEGER,
    TICOS_IOT_VAL_TYPE_FLOAT,
    TICOS_IOT_VAL_TYPE_STRING,
    TICOS_IOT_VAL_TYPE_ENUM,
    TICOS_IOT_VAL_TYPE_TIMESTAMP,
    TICOS_IOT_VAL_TYPE_DURATION,
    TICOS_IOT_VAL_TYPE_MAX,
} ti_iot_val_type_t;

typedef struct {
    time_t start;
    time_t end;
} ti_iot_val_duration_t;

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

#include <_ti_cfg_suffix.h>
