#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include <ti_core.h>
#include <ti_iot.h>
#include <ti_iot_hal.h>
#include <ti_iot_api.h>

#define IOT_CONFIG_IOTHUB_FQDN            "hub.ticos.cn"
#define IOT_CONFIG_DEVICE_ID              "SLC1"
#define IOT_CONFIG_PRODUCT_ID             "HITXM3K4IE"
#define PROPERTY_TOPIC                    "devices/" IOT_CONFIG_DEVICE_ID "/twin/patch/desired"

static esp_mqtt_client_handle_t mqtt_client;

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
int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
}

/**
 * @brief 获取设备 ID 的接口
 * @note  Ticos SDK 会调用此接口，获取设备 ID。用户可以定义 IOT_CONFIG_DEVICE_ID 宏, 或者改写此函数将设备 ID 从其它地方输入
 */
const char *ti_iot_get_device_id(void)
{
    return IOT_CONFIG_DEVICE_ID;
}

/**
 * @brief 获取 IoT Hub 域名的接口
 * @note  Ticos SDK 会调用此接口，获取IoT Hub 域名。用户可以定义 IOT_CONFIG_IOTHUB_FQDN 宏, 或者改写此函数将域名从其它地方输入
 */
const char *ti_iot_get_mqtt_fqdn(void){
    return IOT_CONFIG_IOTHUB_FQDN;
}

/**
 * @brief 获取产品 ID 的接口
 * @note  Ticos SDK 会调用此接口，获取产品 ID。用户可以定义 IOT_CONFIG_PRODUCT_ID 宏, 或者改写此函数将产品 ID 从其它地方输入
 */
const char *ti_iot_get_product_id(void){
    return IOT_CONFIG_PRODUCT_ID;
}

/**
 * @brief 平台相关mqtt事件回调接口
 * @note  当使用mqtt client连接云端成功后，会产生MQTT_EVENT_CONNECTED事件，此时用户需要订阅属性相关的topic(由宏PROPERTY_TOPIC定义)
 * 当client从云端接收到数据后，会产生MQTT_EVENT_DATA事件，此时用户需要回调ti_iot_property_receive将数据传给sdk进行处理
 */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  int r;
  switch (event->event_id)
  {
    case MQTT_EVENT_ERROR:
      printf("MQTT event MQTT_EVENT_ERROR\n");
      break;
    case MQTT_EVENT_CONNECTED:
      printf("MQTT event MQTT_EVENT_CONNECTED\n");
      r = esp_mqtt_client_subscribe(mqtt_client, PROPERTY_TOPIC, 1);
      if (r == -1)
      {
        printf("Could not subscribe for cloud-to-device messages.\n");
      }
      else
      {
        printf("Subscribed for cloud-to-device messages; message id:\n");
      }
      break;
    case MQTT_EVENT_DISCONNECTED:
      printf("MQTT event MQTT_EVENT_DISCONNECTED\n");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      printf("MQTT event MQTT_EVENT_SUBSCRIBED\n");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      printf("MQTT event MQTT_EVENT_UNSUBSCRIBED\n");
      break;
    case MQTT_EVENT_PUBLISHED:
      printf("MQTT event MQTT_EVENT_PUBLISHED\n");
      break;
    case MQTT_EVENT_DATA:
      printf("MQTT event MQTT_EVENT_DATA\n");
      ti_iot_property_receive(event->data, event->data_len);
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      printf("MQTT event MQTT_EVENT_BEFORE_CONNECT\n");
      break;
    default:
      printf("MQTT event UNKNOWN\n");
      break;
  }

  return ESP_OK;
}

/**
 * @brief 该函数为平台相关的mqtt函数，用户需要根据不同平台进行实现。需实现的功能为完成 MQTT client 初始化，供后续 MQTT 请求使用
 * @param mqtt_client_id  MQTT 客户端 ID
 * @param mqtt_username MQTT 用户名
 */
int hal_mqtt_client_init(const char *mqtt_client_id, const char *mqtt_username)
{
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = "mqtt://" IOT_CONFIG_IOTHUB_FQDN;
  mqtt_config.port = 1883;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    printf("Failed creating mqtt client\n");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    printf("Could not start mqtt client; error code:\n");
    return 1;
  }
  else
  {
    printf("MQTT client started\n");
    return 0;
  }
}

 /**
 * @brief 停止ti iot cloud服务
 * @note  该函数停止mqtt client与云端的连接, 需要用户根据平台实现
 */
void ti_iot_cloud_stop()
{
  esp_mqtt_client_stop(mqtt_client);
}
