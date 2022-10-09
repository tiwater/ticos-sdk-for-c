#include "ti_thingmodel.h"

const ti_iot_prop_info_t ti_iot_prop_info_get_tab[] = {
    {"pressure", TICOS_IOT_VAL_TYPE_INTEGER},
    {"temperature", TICOS_IOT_VAL_TYPE_FLOAT},
    {"oxygen", TICOS_IOT_VAL_TYPE_FLOAT},
    {"warn_info", TICOS_IOT_VAL_TYPE_STRING},
};

const ti_iot_prop_info_t ti_iot_prop_info_set_tab[] = {
    {"oxygen", TICOS_IOT_VAL_TYPE_FLOAT},
    {"warn_info", TICOS_IOT_VAL_TYPE_STRING},
};

const void *ti_iot_get_func_tab[] = {
    ti_iot_get_pressure,
    ti_iot_get_temperature,
    ti_iot_get_oxygen,
    ti_iot_get_warn_info,
};

const void *ti_iot_set_func_tab[] = {
    ti_iot_set_oxygen,
    ti_iot_set_warn_info,
};

const int read_prop_cnt = 4;
const int write_prop_cnt = 2;

float ti_iot_get_temperature()
{
    //#error "add your code!"
    return 12.12;
}

int ti_iot_get_pressure()
{
    //#error "add your code!"
    return 1022;
}

float ti_iot_get_oxygen()
{
    //#error "add your code!"
    return 20.1;
}

const char *ti_iot_get_warn_info()
{
    return "add your code!";
}

void ti_iot_set_oxygen(float oxygen)
{
    //#error "add your code!"
}

void ti_iot_set_warn_info(const char *warn_info)
{
    //#error "add your code!"
}
