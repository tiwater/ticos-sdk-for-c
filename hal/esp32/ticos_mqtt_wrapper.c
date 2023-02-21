#include <ticos_api.h>
#include <mqtt_client.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_https_ota.h>
#include <esp_partition.h>
#include "ticos_esp_storage.h"
#include "ticos_import.h"
static const char *TAG = "ticos-mqtt-wrapper";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool ticos_mqtt_connected;
static void (*ota_finnish_callback)(void*) = NULL;
extern esp_ticos_ota_info_t ticos_ota_info;
extern OTAUpdateStatus_t   ota_update;
extern uint8_t progress;
#define TICLOUD_ERROR_GOTO(con, lable, format, ...) do { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, format, ##__VA_ARGS__); \
            goto lable; \
        } \
    } while(0)

#define TICLOUD_ERROR_BREAK(con, format, ...) { \
        if (con) { \
            if(*format != '\0') \
                ESP_LOGW(TAG, format, ##__VA_ARGS__); \
            break; \
        } \
    }
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
    if (!mqtt_client)
      return -1;
    ESP_LOGI(TAG, "mqtt_publish, topic: %s,data: %s, len: %d, qos: %d, retain: %d ", topic, data, len, qos, retain );
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
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
    if (!mqtt_client)
      return -1;
    ESP_LOGI(TAG, "subscribe ,topic = %s, qos = %d ", topic,  qos );
    return esp_mqtt_client_subscribe(mqtt_client, topic, qos);
}

/**
 * @brief 平台相关mqtt事件回调接口
 * @note  当使用mqtt client连接云端成功后，会产生MQTT_EVENT_CONNECTED事件，
 * 此时用户需要调用ticos_mqtt_subscribe()订阅和云端通信相关的topic
 * 当client从云端接收到数据后，会产生MQTT_EVENT_DATA事件，
 * 此时用户需要回调ticos_msg_recv()将数据传给sdk进行处理
 */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGW(TAG, "MQTT event MQTT_EVENT_CONNECTED\n");
      ticos_event_notify(TICOS_EVENT_CONNECT);
      ticos_mqtt_subscribe();
      ticos_system_user_bind();
      ticos_mqtt_connected = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      ticos_event_notify(TICOS_EVENT_DISCONNECT);
      ticos_mqtt_connected = false;
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
      ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
      ticos_msg_recv(event->topic, event->topic_len, event->data, event->data_len);
      break;
    default:
      ESP_LOGW(TAG, "mqtt event: id = %d\n", event->event_id);
      break;
  }
  

  return ESP_OK;
}

/**
 * @brief 启动平台相关的mqtt服务
 * @note  用户需要根据平台实现此函数，提供一个mqtt的客户端。ticos sdk会调用此接口连接到云端
 */
int ticos_hal_mqtt_start(const char *url, int port, const char *client_id, const char *user_name, const char *passwd)
{
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = url;
  mqtt_config.port = port;
  mqtt_config.client_id = client_id;
  mqtt_config.username = user_name;
  mqtt_config.password = passwd;

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  ESP_LOGW(TAG, "iothub mqtt config:");
  ESP_LOGW(TAG, "uri %s:%d", mqtt_config.uri,mqtt_config.port);
  ESP_LOGW(TAG, "client_id: %s", mqtt_config.client_id);
  ESP_LOGW(TAG, "username: %s", mqtt_config.username);
  ESP_LOGW(TAG, "password: %s", mqtt_config.password);
  mqtt_client = esp_mqtt_client_init(&mqtt_config);
  ticos_mqtt_connected = false;
  if (mqtt_client == NULL)
  {
    ESP_LOGW(TAG, "Failed creating mqtt client\n");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    ESP_LOGW(TAG, "Could not start mqtt client; error code:\n");
    return 1;
  }
  else
  {
    ESP_LOGW(TAG, "MQTT client started\n");
    return 0;
  }
}

/**
 * @brief 停止平台相关的mqtt服务
 * @note  该函数停止mqtt客户端与云端的连接, 需要用户根据平台实现。ticos sdk停止时会调用此接口
 */
void ticos_hal_mqtt_stop()
{
  if (!mqtt_client)
    return;
  ticos_mqtt_connected = false;
  esp_mqtt_client_stop(mqtt_client);
  mqtt_client = NULL;
}

int ticos_hal_mqtt_connected(void)
{
    return ticos_mqtt_connected;
}

static esp_err_t validate_image_header(esp_ticos_ota_info_t *ota_info, esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) { 
        ESP_LOGE(TAG, "<ESP_TICLOUD_ERR_INVALID_ARG> !");
        return ESP_ERR_INVALID_ARG;
    }
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;

    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGD(TAG, "Running firmware version: %s", running_app_info.version);
    }

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current running version is same as the new. We will not continue the update.");

        if (ota_info) {
            ticloud_ota_report_update(OTA_UPDATE_FAIL);
        }

        return ESP_FAIL;
    }

    if (memcmp(new_app_info->project_name, running_app_info.project_name, sizeof(new_app_info->project_name)) != 0) {
        ESP_LOGW(TAG, "OTA Image built for Project: %s. Expected: %s",
                 new_app_info->project_name, running_app_info.project_name);

            ticloud_ota_report_update(OTA_UPDATE_FAIL);
        return ESP_FAIL;
    }

    return ESP_OK;
}
static void ticloud_iotbub_ota_task(void *arg)
{
    esp_err_t ota_finish_err = 0;
    esp_ticos_ota_info_t *ota_info = (esp_ticos_ota_info_t *)arg;
    esp_http_client_config_t http_config = {
        .url = ota_info->url,
        .timeout_ms = 5000,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .skip_cert_common_name_check = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    vTaskDelay( 5000 / portTICK_PERIOD_MS );

    /*< Using a warning just to highlight the message */
    ESP_LOGW(TAG, "Starting OTA. This may take time.");
    ticloud_ota_report_update(OTA_UPDATE_DOWNLOAD);
    ticloud_ota_report_progress(0);
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        ticloud_ota_report_update(OTA_UPDATE_FAIL);
        goto EXIT;
    }

    esp_app_desc_t app_desc = {0};
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        ticloud_ota_report_update(OTA_UPDATE_FAIL);
        goto EXIT;
    }

    err = validate_image_header(ota_info, &app_desc);
    TICLOUD_ERROR_GOTO(err != ESP_OK, EXIT, "image header verification failed");

    for (int report_percent = 10;;) {
        static int last_size = 0;
        err = esp_https_ota_perform(https_ota_handle);
        TICLOUD_ERROR_BREAK(err != ESP_ERR_HTTPS_OTA_IN_PROGRESS, "esp_https_ota_perform");

        ota_info->download_size = esp_https_ota_get_image_len_read(https_ota_handle);
        TICLOUD_ERROR_GOTO(ota_info->download_size == last_size, EXIT, "esp_https_ota_perform");
        ota_info->download_percent = ota_info->download_size * 100 / ota_info->file_size;
        ticloud_ota_report_progress(ota_info->download_percent);
        last_size = ota_info->download_size;

        if (report_percent == ota_info->download_percent) {
            ESP_LOGI(TAG, "Downloading Firmware Image, size: %d, percent: %d%%", ota_info->download_size, ota_info->download_percent);
            report_percent += 10;
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    }

EXIT:
    ota_finish_err = esp_https_ota_finish(https_ota_handle);

    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "OTA upgrade successful.");
        ticloud_ota_report_update(OTA_UPDATE_SUCCESS);
        if( ota_finnish_callback ){
            ota_finnish_callback(NULL);
        }
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            ticloud_ota_report_update(OTA_UPDATE_FAIL);
        } else {
            /* Not reporting status here, because relevant error will already be reported
             * in some earlier step
             */
            ticloud_ota_report_update(OTA_UPDATE_FAIL);
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed %d", ota_finish_err);
        }
    }

    vTaskDelete(NULL);
}
void ticos_ota_start_update()
{
    if(ota_update == OTA_UPDATE_DOWNLOAD || ota_update == OTA_UPDATE_FLASH) {
        ESP_LOGE(TAG, "OTA on going");
        return;
    }
    xTaskCreatePinnedToCore(ticloud_iotbub_ota_task, "ticos_ota", 4 * 1024, &ticos_ota_info, 1, NULL, 0);
    return;
}




char *ticloud_ota_version_get()
{
    if(strlen(ticos_ota_info.version) > 0) return ticos_ota_info.version;
    return esp_version_get();
}

void ticloud_ota_finnish_callback_register( void (*cb)(void*) ){
    ota_finnish_callback = cb;
}
