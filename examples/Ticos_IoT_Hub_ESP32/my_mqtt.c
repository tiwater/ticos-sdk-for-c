#include "my_mqtt.h"
#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include <ti_iot_api.h>

static esp_mqtt_client_handle_t mqtt_client;
static char property_topic[128];

/**
 * @brief mqtt客户端向云端推送数据的接口
 * @note  ticos sdk会调用此接口，完成数据的上传。需要用户实现此函数
 * @param[in] topic 上报信息的topic
 * @param[in] data 上报的数据内容
 * @param[in] len  上报的数据长度
 * @return TI_OK for success, other for fail.
 */
int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len) {
    return esp_mqtt_client_publish(mqtt_client, topic, data, len, 1, 0);
}

/**
 * @brief 平台相关mqtt事件回调接口
 * @note  当使用mqtt client连接云端成功后，会产生MQTT_EVENT_CONNECTED事件，此时用户需要订阅属性相关的topic
 * 当client从云端接收到数据后，会产生MQTT_EVENT_DATA事件，此时用户需要回调ti_iot_property_receive将数据传给sdk进行处理
 */
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
#else
static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t id, void *data) {
#endif
    int r;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    int32_t id = event->event_id;
#else
    esp_mqtt_event_handle_t event = data;
#endif
    switch (id) {
        case MQTT_EVENT_ERROR:
            printf("MQTT event MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_CONNECTED:
            printf("MQTT event MQTT_EVENT_CONNECTED");
            r = esp_mqtt_client_subscribe(mqtt_client, property_topic, 1);
            if (r == -1) {
                printf("Could not subscribe for cloud-to-device messages.");
            } else {
                printf("Subscribed for cloud-to-device messages.");
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
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    return ESP_OK;
#endif
}

/**
 * @brief 启动ti iot cloud服务
 * @note  该函数启动mqtt client与云端的连接, 需要用户根据平台实现
 * @param[in] mqtt_fqdn
 * @param[in] product_id
 * @param[in] device_id
 * @return NDOA_OK if success, else fail
 */
int my_mqtt_start(const char* mqtt_fqdn,
                    const char* product_id,
                    const char* device_id) {
    ti_iot_client_init(mqtt_fqdn, product_id, device_id);
    snprintf(property_topic, sizeof(property_topic),
            "devices/%s/twin/patch/desired", device_id);

    const char* mqtt_client_id = ti_iot_mqtt_client_id();
    const char* mqtt_username  = ti_iot_mqtt_username();

    char hub_uri[128] = {};
    snprintf(hub_uri, sizeof(hub_uri), "mqtt://%s", mqtt_fqdn);
    esp_mqtt_client_config_t mqtt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .uri = hub_uri,
        .port = 1883,
        .client_id = mqtt_client_id,
        .username = mqtt_username,
        .keepalive = 30,
        .disable_clean_session = false,
        .disable_auto_reconnect = false,
        .event_handle = mqtt_event_handler,
#else
        .broker.address.uri = hub_uri,
        .broker.address.port = 1883,
        .credentials.client_id = mqtt_client_id,
        .credentials.username = mqtt_username,
        .session.keepalive = 30,
        .session.disable_clean_session = false,
        .network.disable_auto_reconnect = false,
#endif
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL) {
        printf("Failed creating mqtt client");
        return 1;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
#endif

    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK) {
        printf("Could not start mqtt client; error code:");
        return 1;
    }

    printf("MQTT client started");
    return 0;
}

/**
 * @brief 停止ti iot cloud服务
 * @note  该函数停止mqtt client与云端的连接, 需要用户根据平台实现
 * @return NDOA_OK if success, else fail
 */
int my_mqtt_stop(void)
{
    esp_mqtt_client_stop(mqtt_client);
    mqtt_client = NULL;
    return 0;
}
