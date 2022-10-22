#ifndef _ti_IOT_HAL_H
#define _ti_IOT_HAL_H

#include <_ti_cfg_prefix.h>

/**
 * @brief mqtt客户端向云端推送数据的接口
 * @note  ticos sdk会调用此接口，完成数据的上传。需要用户实现此函数
 * @param topic 上报信息的topic
 * @param data 上报的数据内容
 * @param len  上报的数据长度
 * @param qos  通信质量
 * @param retain retain flag
 * @return TI_OK for success, other for fail.
 */
int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);

/**
 * @brief 获取设备ID的接口
 * @note  ticos sdk会调用此接口，获取设备ID。用户可以定义IOT_CONFIG_DEVICE_ID宏, 或者改写此函数将设备ID从其它地方输入
 */
const char *ti_iot_get_device_id(void);

#include <_ti_cfg_suffix.h>

#endif // _ti_IOT_HAL_H
