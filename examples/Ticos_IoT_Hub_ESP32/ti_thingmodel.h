/*************************************************************************
  * @file ti_thingmodel.h 物模型代码文件
  * @generate date: 2022-10-19 16:42:13
  * 此文件为自动生成，请不要更改文件内容
  ************************************************************************/

#pragma once

#include <_ti_cfg_prefix.h>
#include "ti_thingmodel_type.h"

typedef enum {
    TICOS_IOT_TELEMETRY_pressure,
    TICOS_IOT_TELEMETRY_temperature,
    TICOS_IOT_TELEMETRY_oxygen,
    TICOS_IOT_TELEMETRY_warn_info,
    TICOS_IOT_TELEMETRY_MAX,
} ti_iot_telemetry_t;

int ti_iot_telemetry_pressure();
float ti_iot_telemetry_temperature();
float ti_iot_telemetry_oxygen();
const char* ti_iot_telemetry_warn_info();

typedef enum {
    TICOS_IOT_PROPERTY_switch,
    TICOS_IOT_PROPERTY_light,
    TICOS_IOT_PROPERTY_DebugInfo,
    TICOS_IOT_PROPERTY_MAX,
} ti_iot_property_t;

int ti_iot_property_switch_upload();
int ti_iot_property_switch_download(int switch_);
int ti_iot_property_light_upload();
int ti_iot_property_light_download(int light);
const char *ti_iot_property_DebugInfo_upload();
int ti_iot_property_DebugInfo_download(const char *DebugInfo);

typedef enum {
    TICOS_IOT_COMMAND_oxygen,
    TICOS_IOT_COMMAND_temperature,
    TICOS_IOT_COMMAND_MAX,
} ti_iot_command_t;

int ti_iot_command_oxygen(float oxygen);
int ti_iot_command_temperature(float temperature);

#include <_ti_cfg_suffix.h>
