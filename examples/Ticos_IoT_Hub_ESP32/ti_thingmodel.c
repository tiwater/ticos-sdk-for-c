#include "ti_thingmodel.h"

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

int ti_iot_property_humi_upload()
{
    return 76;
}

int ti_iot_property_light_upload()
{
    return 1;
}

int ti_iot_property_light_download(int light)
{
    // op
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
    {"humi", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_property_humi_upload, NULL},
    {"light", TICOS_IOT_VAL_TYPE_INTEGER, ti_iot_property_light_upload, ti_iot_property_light_download},
};

const ti_iot_command_info_t ti_iot_command_tab[] = {
    {"oxygen", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_command_oxygen},
    {"temperature", TICOS_IOT_VAL_TYPE_FLOAT, ti_iot_command_temperature},
};
