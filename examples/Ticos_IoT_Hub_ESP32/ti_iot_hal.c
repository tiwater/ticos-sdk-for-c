#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include <ti_core.h>
#include <ti_iot.h>
#include "ti_iot_api.h"

#define IOT_CONFIG_IOTHUB_FQDN            "hub.ticos.cn"
#define IOT_CONFIG_DEVICE_ID              "TEST002"
#define IOT_CONFIG_PRODUCT_ID             "BOB45WX7H4"

#define PROPERTY_TOPIC    "devices/" IOT_CONFIG_DEVICE_ID "/twin/patch/desired"

#define TICOS_SDK_CLIENT_USER_AGENT "c%2F" TI_SDK_VERSION_STRING "(ard;esp32)"

static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtt://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const char* product_id = IOT_CONFIG_PRODUCT_ID;
static const int mqtt_port = 1883; // TI_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static esp_mqtt_client_handle_t mqtt_client;
static ti_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];

int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
}

const char *ti_iot_get_device_id(void)
{
    return IOT_CONFIG_DEVICE_ID;
}

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

static void initializeIoTHubClient()
{
  ti_iot_hub_client_options options = ti_iot_hub_client_options_default();
  options.user_agent = TI_SPAN_FROM_STR(TICOS_SDK_CLIENT_USER_AGENT);

  if (ti_result_failed(ti_iot_hub_client_init(
          &client,
          ti_span_create((uint8_t*)host, strlen(host)),
          ti_span_create((uint8_t*)device_id, strlen(device_id)),
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
}

static int initializeMqttClient()
{
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
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

void ti_iot_cloud_start()
{
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

