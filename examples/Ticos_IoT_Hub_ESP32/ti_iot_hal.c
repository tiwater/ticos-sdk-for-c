#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include <ti_core.h>
#include <ti_iot.h>
#include "ti_iot_api.h"

#define IOT_CONFIG_IOTHUB_FQDN            "hub.ticos.cn"
#define IOT_CONFIG_DEVICE_ID              "TEST002"
#define IOT_CONFIG_PRODUCT_ID             "BOB45WX7H4"
#define PROPERTY_TOPIC                    "devices/" IOT_CONFIG_DEVICE_ID "/twin/patch/desired"

static esp_mqtt_client_handle_t mqtt_client;
static ti_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];

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
 * @brief 获取设备ID的接口
 * @note  ticos sdk会调用此接口，获取设备ID。用户可以定义IOT_CONFIG_DEVICE_ID宏, 或者改写此函数将设备ID从其它地方输入
 */
const char *ti_iot_get_device_id(void)
{
    return IOT_CONFIG_DEVICE_ID;
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
      printf("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      printf("MQTT event MQTT_EVENT_CONNECTED");
      r = esp_mqtt_client_subscribe(mqtt_client, PROPERTY_TOPIC, 1);
      if (r == -1)
      {
        printf("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        printf("Subscribed for cloud-to-device messages; message id:");
      }
      break;
    case MQTT_EVENT_DISCONNECTED:
      printf("MQTT event MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      printf("MQTT event MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      printf("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      printf("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      printf("MQTT event MQTT_EVENT_DATA");
      ti_iot_property_receive(event->data, event->data_len);
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      printf("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      printf("MQTT event UNKNOWN");
      break;
  }

  return ESP_OK;
}

 /**
 * @brief 该函数为平台相关的mqtt函数，用户需要根据不同平台进行实现。
 * @note  用户设置好url, port, client, username后，会调用esp_mqtt_client_start()连接到ticos iot hub
 */
static int hal_mqtt_client_init()
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
    printf("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    printf("Could not start mqtt client; error code:");
    return 1;
  }
  else
  {
    printf("MQTT client started");
    return 0;
  }
}

 /**
 * @brief 启动ti iot cloud服务
 * @note  该函数先初始化ti_iot_hub_client，然后调用平台相关的mqtt client连接到云端, 用户需要实现hal_mqtt_client_init函数
 */
void ti_iot_cloud_start()
{
  ti_iot_hub_client_options options = ti_iot_hub_client_options_default();
  options.user_agent = TI_SPAN_FROM_STR("c%2F" TI_SDK_VERSION_STRING "(ard;esp32)");

  if (ti_result_failed(ti_iot_hub_client_init(
          &client,
          TI_SPAN_FROM_STR(IOT_CONFIG_IOTHUB_FQDN),
          TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID),
          &options)))
  {
    printf("Failed initializing Ticos IoT Hub client");
    return;
  }

  ti_span span_client_id = TI_SPAN_FROM_BUFFER(mqtt_client_id);
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR("@@@"));
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR(IOT_CONFIG_PRODUCT_ID));

  ti_span span_username = TI_SPAN_FROM_BUFFER(mqtt_username);
  span_username = ti_span_copy(span_username, TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));

  hal_mqtt_client_init();
}

 /**
 * @brief 停止ti iot cloud服务
 * @note  该函数停止mqtt client与云端的连接, 需要用户根据平台实现
 */
void ti_iot_cloud_stop()
{
  esp_mqtt_client_stop(mqtt_client);
}
