/************************************************************************
  * @file ticos_thingmodel.h
  * @brief 物模型代码文件
  * @date 2023-01-16 14:25:37
  * @note 此文件为自动生成，请不要更改文件内容
  ***********************************************************************/

#ifndef __TICOS_THING_MODEL_H
#define __TICOS_THING_MODEL_H

#include "ticos_thingmodel_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    TICOS_TELEMETRY_MAX,
} ticos_telemetry_t;


typedef enum {
    TICOS_PROPERTY_volume,
    TICOS_PROPERTY_brightness,
    TICOS_PROPERTY_sleep_time,
    TICOS_PROPERTY_skin_detection_start,
    TICOS_PROPERTY_spray_enable,
    TICOS_PROPERTY_spray_begin,
    TICOS_PROPERTY_spray_duration,
    TICOS_PROPERTY_battery,
    TICOS_PROPERTY_user_id,
    TICOS_PROPERTY_user_name,
    TICOS_PROPERTY_env_temperature,
    TICOS_PROPERTY_env_humidity,
    TICOS_PROPERTY_music_type,
    TICOS_PROPERTY_music_start,
    TICOS_PROPERTY_MAX,
} ticos_property_t;

typedef enum {
    TICOS_COMMAND_MAX,
} ticos_command_t;

int ticos_property_volume_send(void);
int ticos_property_volume_recv(int volume_);
int ticos_property_brightness_send(void);
int ticos_property_brightness_recv(int brightness_);
float ticos_property_sleep_time_send(void);
int ticos_property_sleep_time_recv(float sleep_time_);
bool ticos_property_skin_detection_start_send(void);
int ticos_property_skin_detection_start_recv(bool skin_detection_start_);
bool ticos_property_spray_enable_send(void);
int ticos_property_spray_enable_recv(bool spray_enable_);
const char* ticos_property_spray_begin_send(void);
int ticos_property_spray_begin_recv(const char* spray_begin_);
int ticos_property_spray_duration_send(void);
int ticos_property_spray_duration_recv(int spray_duration_);
int ticos_property_battery_send(void);
int ticos_property_battery_recv(int battery_);
const char* ticos_property_user_id_send(void);
int ticos_property_user_id_recv(const char* user_id_);
const char* ticos_property_user_name_send(void);
int ticos_property_user_name_recv(const char* user_name_);
float ticos_property_env_temperature_send(void);
int ticos_property_env_temperature_recv(float env_temperature_);
float ticos_property_env_humidity_send(void);
int ticos_property_env_humidity_recv(float env_humidity_);
int ticos_property_music_type_send(void);
int ticos_property_music_type_recv(int music_type_);
bool ticos_property_music_start_send(void);
int ticos_property_music_start_recv(bool music_start_);
const int ticos_telemetry_cnt = TICOS_TELEMETRY_MAX;
const int ticos_property_cnt = TICOS_PROPERTY_MAX;
const int ticos_command_cnt = TICOS_COMMAND_MAX;

#ifdef __cplusplus
}
#endif

#endif // __TICOS_THING_MODEL_H
