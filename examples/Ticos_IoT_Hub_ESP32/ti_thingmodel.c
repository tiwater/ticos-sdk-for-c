/*************************************************************************
  * @file ti_thingmodel.c 物模型代码文件
  * @generate date: 2022-10-22 17:24:58
  ************************************************************************/

#include "ti_thingmodel.h"

bool ti_iot_property_switch_send(void) {
    return get_switch_state();
}

int ti_iot_property_switch_recv(bool switch_) {
    return 0;
}

bool ti_iot_property_led_send(void) {
    return get_led_light();
}

int ti_iot_property_led_recv(bool led_) {
    printf("receive led: %d\r\n", led_);
    set_led_light(led_);
    return 0;
}

const ti_iot_telemetry_info_t ti_iot_telemetry_tab[] = {
};

const ti_iot_property_info_t ti_iot_property_tab[] = {
    { "switch", TICOS_IOT_VAL_TYPE_BOOLEAN,  ti_iot_property_switch_send, ti_iot_property_switch_recv },
    { "led", TICOS_IOT_VAL_TYPE_BOOLEAN,  ti_iot_property_led_send, ti_iot_property_led_recv },
};

const ti_iot_command_info_t ti_iot_command_tab[] = {
};

const int ti_iot_telemetry_cnt = TICOS_IOT_TELEMETRY_MAX;
const int ti_iot_property_cnt = TICOS_IOT_PROPERTY_MAX;
const int ti_iot_command_cnt = TICOS_IOT_COMMAND_MAX;
