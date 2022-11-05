/**
  * @file ticos_mqtt_wrapper.c
  * @note 此文件用于对接任意平台mqtt模块实现 mqtt client 运行
  *       开发者通过填写以下函数族即可开启支持 ticos sdk 运行的 mqtt 客户端
  *       开发者请参考 sdk 任意例程中同名代码文件，完成本模块开发
  */

#include <ticos_api.h>

/**
 * @brief mqtt客户端向云端推送数据的接口
 * @note  ticos sdk会调用此接口，完成数据的上传。需要用户实现此函数
 * @param topic 上报信息的topic
 * @param data 上报的数据内容
 * @param len  上报的数据长度
 * @param qos  通信质量
 * @param retain retain flag
 * @return 0 for success, other for fail.
 */
int ticos_hal_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    // TODO
    return -1;
}

/**
 * @brief mqtt客户端订阅云端topic接口
 * @note  ticos sdk会调用此接口，完成指定topic的订阅。需要用户实现此函数
 * @param topic 需要订阅的topic
 * @param qos  通信质量
 * @return 0 for success, other for fail.
 */
int ticos_hal_mqtt_subscribe(const char *topic, int qos)
{
    // TODO
    return -1;
}

/**
 * @brief 平台相关mqtt事件回调接口例子
 * @note  当使用连接云端成功后，会产生MQTT_EVENT_CONNECTED事件，
 *        此时用户需要调用ticos_mqtt_subscribe()订阅和云端通信相关的topic
 *        当client从云端接收到数据后，会产生MQTT_EVENT_DATA事件，
 *        此时用户需要回调ticos_msg_recv()将数据传给sdk进行处理
 */
#if 0
static void mqtt_event_handler(mqtt_event_handle_t event)
{
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      printf("MQTT event MQTT_EVENT_CONNECTED");
      ticos_mqtt_subscribe();
      break;
    case MQTT_EVENT_DATA:
      printf("MQTT event MQTT_EVENT_DATA");
      ticos_msg_recv(event->topic, event->data, event->data_len);
      break;
    default:
      printf("mqtt event: id = %d\n", event->event_id);
      break;
  }
}
#endif

/**
 * @brief 启动平台相关的mqtt服务
 * @note  用户需要根据平台实现此函数，提供一个mqtt的客户端。ticos sdk会调用此接口连接到云端
 * @param mqtt_uri mqtt 云端 uri, 如 mqtt://host.my_cloud.cc
 * @param mqtt_port mqtt 通讯端口
 * @param mqtt_client_id mqtt 客户端 id
 * @param mqtt_user_name mqtt 客户端名称
 */
int ticos_hal_mqtt_start(const char *mqtt_uri,
                         int         mqtt_port,
                         const char *mqtt_client_id,
                         const char *mqtt_user_name)
{
    // TODO
    return -1;
}

/**
 * @brief 停止平台相关的mqtt服务
 * @note  该函数停止mqtt客户端与云端的连接, 需要用户根据平台实现。ticos sdk停止时会调用此接口
 */
void ticos_hal_mqtt_stop(void)
{
    // TODO
}
