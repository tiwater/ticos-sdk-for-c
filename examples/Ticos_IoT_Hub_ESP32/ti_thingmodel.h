#pragma once

#include "ti_core.h"

#include <_ti_cfg_prefix.h>
// 物模型的数据类型，固定不变
typedef enum {
    TICOS_IOT_VAL_TYPE_INVALID = 0,
    TICOS_IOT_VAL_TYPE_BOOLEAN,
    TICOS_IOT_VAL_TYPE_INTEGER,
    TICOS_IOT_VAL_TYPE_FLOAT,
    TICOS_IOT_VAL_TYPE_STRING,
    TICOS_IOT_VAL_TYPE_ENUM,
} ti_iot_val_type_t;

typedef struct {
    const char *id;
    ti_iot_val_type_t type;
} ti_iot_prop_info_t;

ti_span ti_iot_property_msgs_pack(ti_span payload);
ti_span ti_iot_property_msg_pack_by_id(int index, ti_span payload);
ti_span ti_iot_property_msg_pack_by_name(const char *prop, ti_span payload);

//======================================= 以下为变动的部分

// 根据物模型生成相应的属性
typedef enum {
    TICOS_IOT_PROP_pressure,
    TICOS_IOT_PROP_temperature,
    TICOS_IOT_PROP_oxygen,
    TICOS_IOT_PROP_warn_info,
    TICOS_IOT_PROP_MAX,
} ti_iot_prop_t;

// 生成相应的get函数, 返回值为云端定义的可读的属性类型
int ti_iot_get_pressure();
float ti_iot_get_temperature();
float ti_iot_get_oxygen();
const char *ti_iot_get_warn_info();

// 生成相应的set函数，传入参数为云端定义的可写的属性类型
void ti_iot_set_oxygen(float oxygen);
void ti_iot_set_warn_info(const char *warn_info);

#include <_ti_cfg_suffix.h>
