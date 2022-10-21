#include <cstdlib>
#include <string.h>
#include <time.h>

#include <WiFi.h>
#include <mqtt_client.h>

#include <ti_core.h>
#include <ti_iot.h>
#include "ti_iot_api.h"
#include "ti_thingmodel.h"

#define IOT_CONFIG_WIFI_SSID              "Tiwater"
#define IOT_CONFIG_WIFI_PASSWORD          "Ti210223"
#define IOT_CONFIG_IOTHUB_FQDN            "hub.ticos.cn"
#define IOT_CONFIG_DEVICE_ID              "TEST002"
#define IOT_CONFIG_PRODUCT_ID             "BOB45WX7H4"

#define PROPERTY_TOPIC    "devices/" IOT_CONFIG_DEVICE_ID "/twin/patch/desired"

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define TICOS_SDK_CLIENT_USER_AGENT "c%2F" TI_SDK_VERSION_STRING "(ard;esp32)"

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

// Translate iot_configs.h defines into variables used by the sample
static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;
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

extern "C" int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
}

extern "C" const char *ti_iot_get_device_id(void)
{
    return IOT_CONFIG_DEVICE_ID;
}

static void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

static void initializeTime()
{
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    now = time(nullptr);
  }
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

  // 暂时写死
  ti_span span_client_id = TI_SPAN_FROM_BUFFER(mqtt_client_id);
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR("@@@"));
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR(IOT_CONFIG_PRODUCT_ID));

  ti_span span_username = TI_SPAN_FROM_BUFFER(mqtt_username);
  span_username = ti_span_copy(span_username, TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));

//  size_t client_id_length;
//  if (ti_result_failed(ti_iot_hub_client_get_client_id(
//          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
//  {
//    printf("Failed getting client id");
//    return;
//  }
//
//  if (ti_result_failed(ti_iot_hub_client_get_user_name(
//          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
//  {
//    printf("Failed to get MQTT clientId, return code");
//    return;
//  }
}

static int initializeMqttClient()
{
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;

//  printf("MQTT client using X509 Certificate authentication");
//  mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
//  mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
//  mqtt_config.use_secure_element = false;
//  mqtt_config.skip_cert_common_name_check = true;
//  mqtt_config.cert_pem = (const char*)ca_pem;

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

extern "C" void ti_iot_cloud_start()
{
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

