/*************************************************************************
  * @file ti_thingmodel.h 物模型代码文件
  * @generate date: 2022-10-19 16:42:13
  * 此文件为自动生成，请不要更改文件内容
  ************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ticos_thingmodel_type.h"

typedef enum {
    TICOS_TELEMETRY_pressure,
    TICOS_TELEMETRY_temperature,
    TICOS_TELEMETRY_oxygen,
    TICOS_TELEMETRY_warn_info,
    TICOS_TELEMETRY_MAX,
} ticos_telemetry_t;

int ticos_telemetry_pressure();
float ticos_telemetry_temperature();
float ticos_telemetry_oxygen();
const char* ticos_telemetry_warn_info();

typedef enum {
    TICOS_PROPERTY_switch,
    TICOS_PROPERTY_light,
    TICOS_PROPERTY_DebugInfo,
    TICOS_PROPERTY_MAX,
} ticos_property_t;

int ticos_property_switch_send();
int ticos_property_switch_recv(int switch_);
int ticos_property_light_send();
int ticos_property_light_recv(int light);
const char *ticos_property_DebugInfo_send();
int ticos_property_DebugInfo_recv(const char *DebugInfo);

typedef enum {
    TICOS_COMMAND_oxygen,
    TICOS_COMMAND_temperature,
    TICOS_COMMAND_MAX,
} ticos_command_t;

int ticos_command_oxygen(float oxygen);
int ticos_command_temperature(float temperature);

#ifdef __cplusplus
}
#endif
