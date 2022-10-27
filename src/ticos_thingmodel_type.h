#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>

typedef enum {
    TICOS_VAL_TYPE_BOOLEAN,
    TICOS_VAL_TYPE_INTEGER,
    TICOS_VAL_TYPE_FLOAT,
    TICOS_VAL_TYPE_STRING,
    TICOS_VAL_TYPE_ENUM,
    TICOS_VAL_TYPE_TIMESTAMP,
    TICOS_VAL_TYPE_DURATION,
    TICOS_VAL_TYPE_MAX,
} ticos_val_type_t;

typedef struct {
    time_t start;
    time_t end;
} ticos_val_duration_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *func;
} ticos_telemetry_info_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *send_func;
    void *recv_func;
} ticos_property_info_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *func;
} ticos_command_info_t;

#ifdef __cplusplus
}
#endif
