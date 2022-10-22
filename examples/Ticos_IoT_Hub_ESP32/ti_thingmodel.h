/************************************************************************
  * @file ti_thingmodel.h 物模型代码文件
  * @generate date: 2022-10-22 18:42:04
  ***********************************************************************/

#ifndef __TI_THING_MODEL_H
#define __TI_THING_MODEL_H

#include <_ti_cfg_prefix.h>
#include <stdbool.h>
#include "ti_thingmodel_type.h"

typedef enum {
    TICOS_IOT_TELEMETRY_MAX,
} ti_iot_telemetry_t;

typedef enum {
    TICOS_IOT_PROPERTY_switch,
    TICOS_IOT_PROPERTY_led,
    TICOS_IOT_PROPERTY_MAX,
} ti_iot_property_t;

typedef enum {
    TICOS_IOT_COMMAND_MAX,
} ti_iot_command_t;

bool ti_iot_property_switch_send(void);
int ti_iot_property_switch_recv(bool switch_);
bool ti_iot_property_led_send(void);
int ti_iot_property_led_recv(bool led_);

#include <_ti_cfg_suffix.h>

#endif // __TI_THING_MODEL_H
