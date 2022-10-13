// Copyright (c) Tiwater Technologies. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * This is an Arduino-based Ticos IoT Hub sample for ESPRESSIF ESP32 boards.
 * It uses our Ticos Embedded SDK for C to help interact with Ticos IoT.
 * For reference, please visit https://github.com/tiwater/ticos-sdk-for-c.
 *
 * To connect and work with Ticos IoT Hub you need an MQTT client, connecting, subscribing
 * and publishing to specific topics to use the messaging features of the hub.
 * Our ticos-sdk-for-c is an MQTT client support library, helping composing and parsing the
 * MQTT topic names and messages exchanged with the Ticos IoT Hub.
 *
 * This sample performs the following tasks:
 * - Synchronize the device clock with a NTP server;
 * - Initialize our "ti_iot_hub_client" (struct for data, part of our ticos-sdk-for-c);
 * - Initialize the MQTT client (here we use ESPRESSIF's esp_mqtt_client, which also handle the tcp connection and TLS);
 * - Connect the MQTT client (using server-certificate validation, SAS-tokens for client authentication);
 * - Periodically send telemetry data to the Ticos IoT Hub.
 *
 * To properly connect to your Ticos IoT Hub, please fill the information in the `iot_configs.h` file.
 */

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>

// Ticos IoT SDK for C includes
#include <ti_core.h>
#include <ti_iot.h>

// Additional sample headers
#include "SerialLogger.h"
#include "iot_configs.h"
#include "ti_thingmodel.h"

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define TICOS_SDK_CLIENT_USER_AGENT "c%2F" TI_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
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
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint8_t telemetry_payload[256];
static uint32_t telemetry_send_count = 0;

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

static void connectToWiFi()
{
  Logger.Info("Connecting to WIFI SSID " + String(ssid));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Logger.Info("WiFi connected, IP address: " + WiFi.localIP().toString());
}

static void initializeTime()
{
  Logger.Info("Setting time using SNTP");

  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  Logger.Info("Time initialized!");
}

void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  Logger.Info("Received [");
  Logger.Info(topic);
  Logger.Info("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
    int i, r;

    case MQTT_EVENT_ERROR:
      Logger.Info("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      Logger.Info("MQTT event MQTT_EVENT_CONNECTED");

      r = esp_mqtt_client_subscribe(mqtt_client, TI_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.Error("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        Logger.Info("Subscribed for cloud-to-device messages; message id:"  + String(r));
      }

      break;
    case MQTT_EVENT_DISCONNECTED:
      Logger.Info("MQTT event MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      Logger.Info("MQTT event MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      Logger.Info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      Logger.Info("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      Logger.Info("MQTT event MQTT_EVENT_DATA");

      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
      {
        incoming_data[i] = event->topic[i];
      }
      incoming_data[i] = '\0';
      Logger.Info("Topic: " + String(incoming_data));

      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
      {
        incoming_data[i] = event->data[i];
      }
      incoming_data[i] = '\0';
      Logger.Info("Data: " + String(incoming_data));
      ti_iot_command_parse(event->data, event->data_len);

      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Logger.Info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      Logger.Error("MQTT event UNKNOWN");
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
    Logger.Error("Failed initializing Ticos IoT Hub client");
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
//    Logger.Error("Failed getting client id");
//    return;
//  }
//
//  if (ti_result_failed(ti_iot_hub_client_get_user_name(
//          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
//  {
//    Logger.Error("Failed to get MQTT clientId, return code");
//    return;
//  }

  Logger.Info("Client ID: " + String(mqtt_client_id));
  Logger.Info("Username: " + String(mqtt_username));
}

static int initializeMqttClient()
{
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;

//  Logger.Info("MQTT client using X509 Certificate authentication");
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
    Logger.Error("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Logger.Error("Could not start mqtt client; error code:" + start_result);
    return 1;
  }
  else
  {
    Logger.Info("MQTT client started");
    return 0;
  }
}

/*
 * @brief           Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */
static uint32_t getEpochTimeInSecs()
{
  return (uint32_t)time(NULL);
}

static void establishConnection()
{
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

static void getTelemetryPayload(ti_span payload, ti_span* out_payload)
{
  ti_span original_payload = payload;

  payload = ti_span_copy(
      payload, TI_SPAN_FROM_STR("{ \"$dtId\": \""));
  payload = ti_span_copy(payload, TI_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));
  payload = ti_span_copy(payload, TI_SPAN_FROM_STR("\","));

  payload = ti_iot_telemetry_msgs_pack(payload);

  payload = ti_span_copy(payload, TI_SPAN_FROM_STR(", \"$metadata\":{\"$model\":\"dtmi:tiwater:"));
  payload = ti_span_copy(payload, TI_SPAN_FROM_STR(IOT_CONFIG_PRODUCT_ID));
  payload = ti_span_copy(payload, TI_SPAN_FROM_STR(";1\"}"));
  payload = ti_span_copy(payload, TI_SPAN_FROM_STR(" }"));
  payload = ti_span_copy_u8(payload, '\0');

  *out_payload = ti_span_slice(original_payload, 0, ti_span_size(original_payload) - ti_span_size(payload) - 1);
}

static void sendTelemetry()
{
  ti_span telemetry = TI_SPAN_FROM_BUFFER(telemetry_payload);

  Logger.Info("Sending telemetry ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (ti_result_failed(ti_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Logger.Error("Failed ti_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  getTelemetryPayload(telemetry, &telemetry);
  printf("mqtt msg: %s\n", (const char*)ti_span_ptr(telemetry));

  if (esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)ti_span_ptr(telemetry),
          ti_span_size(telemetry),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == 0)
  {
    Logger.Error("Failed publishing");
  }
  else
  {
    Logger.Info("Message published successfully");
  }
}

// Arduino setup and loop main functions.

void setup()
{
  establishConnection();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  else if (millis() > next_telemetry_send_time_ms)
  {
    sendTelemetry();
    next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS;
  }
}
