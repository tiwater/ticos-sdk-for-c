/*************************************************************************
  * @file ticos_thingmodel.c
  * @brief 物模型代码文件
  * @date 2023-01-16 14:25:37
  * @note 请在下面各个telemetry, property和command函数中写入相关的业务逻辑代码
  *       ticos_telemetry_xxx 填写要上报的telemetry的值
  *       ticos_property_xxx_send 填写要上报的property的值
  *       ticos_property_xxx_recv 处理云端下发的property值
  *       ticos_command_xxx 处理云端下发的命令
  ************************************************************************/

#include "ticos_thingmodel.h"
#include "ticos_thingmodel_type.h"


const ticos_telemetry_info_t ticos_telemetry_tab[] = {
};

const ticos_property_info_t ticos_property_tab[] = {
    { "volume", TICOS_VAL_TYPE_INTEGER,  ticos_property_volume_send, ticos_property_volume_recv },
    { "brightness", TICOS_VAL_TYPE_INTEGER,  ticos_property_brightness_send, ticos_property_brightness_recv },
    { "sleep_time", TICOS_VAL_TYPE_DOUBLE,  ticos_property_sleep_time_send, ticos_property_sleep_time_recv },
    { "skin_detection_start", TICOS_VAL_TYPE_BOOLEAN,  ticos_property_skin_detection_start_send, ticos_property_skin_detection_start_recv },
    { "spray_enable", TICOS_VAL_TYPE_BOOLEAN,  ticos_property_spray_enable_send, ticos_property_spray_enable_recv },
    { "spray_begin", TICOS_VAL_TYPE_STRING,  ticos_property_spray_begin_send, ticos_property_spray_begin_recv },
    { "spray_duration", TICOS_VAL_TYPE_INTEGER,  ticos_property_spray_duration_send, ticos_property_spray_duration_recv },
    { "battery", TICOS_VAL_TYPE_INTEGER,  ticos_property_battery_send, ticos_property_battery_recv },
    { "user_id", TICOS_VAL_TYPE_STRING,  ticos_property_user_id_send, ticos_property_user_id_recv },
    { "user_name", TICOS_VAL_TYPE_STRING,  ticos_property_user_name_send, ticos_property_user_name_recv },
    { "env_temperature", TICOS_VAL_TYPE_DOUBLE,  ticos_property_env_temperature_send, ticos_property_env_temperature_recv },
    { "env_humidity", TICOS_VAL_TYPE_DOUBLE,  ticos_property_env_humidity_send, ticos_property_env_humidity_recv },
    { "music_type", TICOS_VAL_TYPE_ENUM,  ticos_property_music_type_send, ticos_property_music_type_recv },
    { "music_start", TICOS_VAL_TYPE_BOOLEAN,  ticos_property_music_start_send, ticos_property_music_start_recv },
};

const ticos_command_info_t ticos_command_tab[] = {
    
};

