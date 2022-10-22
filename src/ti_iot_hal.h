#ifndef _ti_IOT_HAL_H
#define _ti_IOT_HAL_H

#include <_ti_cfg_prefix.h>

/**
 * @brief MQTT 客户端向云端推送数据的接口
 * @note  Ticos SDK 会调用此接口，完成数据的上传。需要用户实现此函数
 * @param topic 上报信息的topic
 * @param data 上报的数据内容
 * @param len  上报的数据长度
 * @param qos  通信质量
 * @param retain retain flag
 * @return TI_OK for success, other for fail.
 */
int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);

/**
 * @brief 获取设备 ID 的接口
 * @note  Ticos SDK 会调用此接口，获取设备 ID。用户可以定义 IOT_CONFIG_DEVICE_ID 宏, 或者改写此函数将设备 ID 从其它地方输入
 */
const char *ti_iot_get_device_id(void);

/**
 * @brief 获取产品 ID 的接口
 * @note  Ticos SDK 会调用此接口，获取产品 ID。用户可以定义 IOT_CONFIG_PRODUCT_ID 宏, 或者改写此函数将产品 ID 从其它地方输入
 */
const char *ti_iot_get_product_id(void);

/**
 * @brief 获取 IoT Hub 域名的接口
 * @note  Ticos SDK 会调用此接口，获取IoT Hub 域名。用户可以定义 IOT_CONFIG_IOTHUB_FQDN 宏, 或者改写此函数将域名从其它地方输入
 */
const char *ti_iot_get_mqtt_fqdn(void);

/**
 * @brief 该函数为平台相关的mqtt函数，用户需要根据不同平台进行实现。需实现的功能为完成 MQTT client 初始化，供后续 MQTT 请求使用
 * @param mqtt_client_id  MQTT 客户端 ID
 * @param mqtt_username MQTT 用户名
 */
int hal_mqtt_client_init(const char *mqtt_client_id, const char *mqtt_username);

#include <_ti_cfg_suffix.h>

#endif // _ti_IOT_HAL_H
