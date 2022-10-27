#ifndef __MY_MQTT_H
#define __MY_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef void (*my_mqtt_client_on_recv_cb) (const char* data, size_t len);

void my_mqtt_client_set_on_recv_cb(my_mqtt_client_on_recv_cb cb);

/**
 * @brief mqtt客户端向云端推送数据的接口
 * @note  ticos sdk会调用此接口，完成数据的上传。需要用户实现此函数
 * @param[in] topic 上报信息的topic
 * @param[in] data 上报的数据内容
 * @param[in] len  上报的数据长度
 * @return true for success, false for fail.
 */
bool my_mqtt_client_publish(const char* topic, const char* data, size_t len);

/**
 * @brief 启动ti iot cloud服务
 * @note  该函数启动mqtt client与云端的连接, 需要用户根据平台实现
 * @param[in] mqtt_fqdn
 * @param[in] mqtt_client_id
 * @param[in] mqtt_username
 * @param[in] device_id
 * @return NDOA_OK if success, else fail
 */
int my_mqtt_client_start(const char* mqtt_fqdn,
                         const char* mqtt_client_id,
                         const char* mqtt_username,
                         const char* device_id);

/**
 * @brief 停止ti iot cloud服务
 * @note  该函数停止mqtt client与云端的连接, 需要用户根据平台实现
 * @return NDOA_OK if success, else fail
 */
int my_mqtt_client_stop(void);

#ifdef __cplusplus
}
#endif

#endif // __MY_MQTT_H
