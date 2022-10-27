/*************************************************************************
  * @file ti_thingmodel.c 物模型代码文件
  * @generate date: 2022-10-19 16:42:13
  * 请在下面各个telemetry, property和command函数中写入相关的业务逻辑代码
  * ticos_telemetry_xxx 填写要上报的telemetry的值
  * ticos_property_xxx_send 填写要上报的property的值
  * ticos_property_xxx_recv 处理云端下发的property值
  * ticos_command_xxx 处理云端下发的命令
  ************************************************************************/

#include "ticos_thingmodel.h"
#include "user_app.h"

int ticos_telemetry_pressure()
{
    return 1023;
}

float ticos_telemetry_temperature()
{
    return 22.4;
}

float ticos_telemetry_oxygen()
{
    return 12.6;
}

const char* ticos_telemetry_warn_info()
{
    return "add your code";
}

int ticos_property_switch_send()
{
    return get_switch_state();
}

int ticos_property_switch_recv(int switch_)
{
    return 0;
}

int ticos_property_light_send()
{
    return get_led_light();
}

int ticos_property_light_recv(int light)
{
    set_led_light(light);
    return 0;
}

const char *ticos_property_DebugInfo_send()
{
    return "Hello, Tiwater";
}

int ticos_property_DebugInfo_recv(const char *DebugInfo)
{
    return 0;
}

int ticos_command_oxygen(float oxygen)
{
    return 0;
}

int ticos_command_temperature(float temperature)
{
    return 0;
}

const ticos_telemetry_info_t ticos_telemetry_tab[] = {
    {"pressure", TICOS_VAL_TYPE_INTEGER, ticos_telemetry_pressure},
    {"temperature", TICOS_VAL_TYPE_FLOAT, ticos_telemetry_temperature},
    {"oxygen", TICOS_VAL_TYPE_FLOAT, ticos_telemetry_oxygen},
    {"warn_info", TICOS_VAL_TYPE_STRING, ticos_telemetry_warn_info},
};

const ticos_property_info_t ticos_property_tab[] = {
    {"switch", TICOS_VAL_TYPE_BOOLEAN, ticos_property_switch_send, ticos_property_switch_recv},
    {"light", TICOS_VAL_TYPE_INTEGER, ticos_property_light_send, ticos_property_light_recv},
    {"DebugInfo", TICOS_VAL_TYPE_STRING, ticos_property_DebugInfo_send, ticos_property_DebugInfo_recv},
};

const ticos_command_info_t ticos_command_tab[] = {
    {"oxygen", TICOS_VAL_TYPE_FLOAT, ticos_command_oxygen},
    {"temperature", TICOS_VAL_TYPE_FLOAT, ticos_command_temperature},
};

const int ticos_telemetry_cnt = TICOS_TELEMETRY_MAX;
const int ticos_property_cnt = TICOS_PROPERTY_MAX;
const int ticos_command_cnt = TICOS_COMMAND_MAX;
