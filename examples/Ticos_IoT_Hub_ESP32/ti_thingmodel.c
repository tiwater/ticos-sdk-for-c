#include "ti_thingmodel.h"
#include <stdio.h>
#include "user_app.h"

int ti_iot_telemetry_pressure()
{
    return 1023;
}

float ti_iot_telemetry_temperature()
{
    return 22.4;
}

float ti_iot_telemetry_oxygen()
{
    return 12.6;
}

const char* ti_iot_telemetry_warn_info()
{
    return "add your code";
}

int ti_iot_property_switch_upload()
{
    return get_switch_state();
}

int ti_iot_property_switch_download(int switch_)
{
    printf("receive switch: %d\r\n", switch_);
    return 0;
}

int ti_iot_property_light_upload()
{
    return get_led_light();
}

int ti_iot_property_light_download(int light)
{
    // op
    printf("receive light: %d\r\n", light);
    set_led_light(light);
    return 0;
}

const char *ti_iot_property_DebugInfo_upload()
{
    return "Hello, Tiwater";
}

int ti_iot_property_DebugInfo_download(const char *DebugInfo)
{
    // op
    printf("recevei DebugInfo: %s\r\n", DebugInfo);
    return 0;
}

int ti_iot_command_oxygen(float oxygen)
{
    // op
    return 0;
}

int ti_iot_command_temperature(float temperature)
{
    printf("[%s:%d] temperature = %f\n", __func__, __LINE__, temperature);
    return 0;
}

const ti_iot_telemetry_info_t ti_iot_telemetry_tab[] = {
    {"pressure", TICOS_IOT_VAL_TYPE_INTEGER, ti_iot_telemetry_pressure},
    {"temperature", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_telemetry_temperature},
    {"oxygen", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_telemetry_oxygen},
    {"warn_info", TICOS_IOT_VAL_TYPE_STRING, ti_iot_telemetry_warn_info},
};

const ti_iot_property_info_t ti_iot_property_tab[] = {
    {"switch", TICOS_IOT_VAL_TYPE_BOOLEAN, ti_iot_property_switch_upload, ti_iot_property_switch_download},
    {"light", TICOS_IOT_VAL_TYPE_INTEGER, ti_iot_property_light_upload, ti_iot_property_light_download},
    {"DebugInfo", TICOS_IOT_VAL_TYPE_STRING, ti_iot_property_DebugInfo_upload, ti_iot_property_DebugInfo_download},
};

const ti_iot_command_info_t ti_iot_command_tab[] = {
    {"oxygen", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_command_oxygen},
    {"temperature", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_command_temperature},
};

const int ti_iot_telemetry_cnt = TICOS_IOT_TELEMETRY_MAX;
const int ti_iot_property_cnt = TICOS_IOT_PROPERTY_MAX;
const int ti_iot_command_cnt = TICOS_IOT_COMMAND_MAX;
