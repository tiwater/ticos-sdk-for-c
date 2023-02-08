#include <stdio.h>
#include <string.h>
#include "ticos_api.h"

int ticos_hal_mqtt_start(const char *url, int port, const char *client_id, const char *user_name, const char *passwd);
void ticos_hal_mqtt_stop();
int ticos_hal_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);
int ticos_hal_mqtt_subscribe(const char *topic, int qos);
int ticos_hal_mqtt_connected(void);
static char ticos_client_id[128];
static char ticos_device_id[128];
static char ticos_device_secret[64];
static char ticos_command_request_topic[128];
static char ticos_property_desired_topic[128];
static char ticos_ota_cloud_response[128];
static char ticos_ota_cloud_request[128];
char ticos_property_report_topic[128];
char ticos_telemery_topic[128];
char ticos_system_user_bind_topic[128];
char ticos_system_user_response_topic[128];
char ticos_system_user_id[128];
/*
ota
*/
esp_ticos_ota_info_t ticos_ota_info = {0};
OTAMeasureStatus_t  ota_measure = OTA_MEASURE_IDLE;
OTAUpdateStatus_t   ota_update = OTA_UPDATE_IDLE;
uint8_t progress = 0;
int ticos_cloud_start(const char* product_id, const char* device_id, const char *device_secret)
{
    sprintf(ticos_client_id, "%s@@@%s", device_id, product_id);
    sprintf(ticos_device_id, "%s", device_id);
    sprintf(ticos_device_secret, "%s", device_secret);
    sprintf(ticos_command_request_topic, "devices/%s@@@%s/commands/request", device_id, product_id);
    sprintf(ticos_property_desired_topic, "devices/%s@@@%s/twin/desired", device_id, product_id);
    sprintf(ticos_property_report_topic, "devices/%s@@@%s/twin/reported", device_id, product_id);
    sprintf(ticos_system_user_bind_topic,"devices/%s@@@%s/user/bind", device_id, product_id);
    sprintf(ticos_system_user_response_topic,"devices/%s@@@%s/user/response", device_id, product_id);
    sprintf(ticos_telemery_topic, "devices/%s@@@%s/telemetry", device_id, product_id);
    snprintf(ticos_ota_cloud_response, 256, "devices/%s@@@%s/cloud/response", device_id, product_id);
    snprintf(ticos_ota_cloud_request, 256, "devices/%s@@@%s/cloud/request", device_id, product_id);
    return ticos_hal_mqtt_start("mqtt://hub.ticos.cn", 1883, ticos_client_id, ticos_device_id, ticos_device_secret);
}

void ticos_cloud_set_bind_user_ID(char *token, int token_len)
{
    memset(ticos_system_user_id, 0 ,sizeof(ticos_system_user_id));
    strncpy(ticos_system_user_id, token, token_len);
}

void ticos_cloud_stop()
{
    ticos_hal_mqtt_stop();
}
int ticos_cloud_connected()
{
    return ticos_hal_mqtt_connected();
}
int ticos_mqtt_subscribe()
{
    int ret = ticos_hal_mqtt_subscribe(ticos_property_desired_topic, 1);
    ret |= ticos_hal_mqtt_subscribe(ticos_system_user_response_topic, 1);
    ret |= ticos_hal_mqtt_subscribe(ticos_ota_cloud_response, 1);
    //if (!ret)
       // return ticos_hal_mqtt_subscribe(ticos_command_request_topic, 1)|ticos_hal_mqtt_subscribe(ticos_system_user_response_topic, 1);
    return ret;
}

void ticos_command_receive(const char *dat, int len);
void ticos_property_receive(const char *dat, int len);
void ticos_system_user_response_receive(const char *dat, int len);

void ticos_msg_recv(const char *topic, int topic_len, const char *dat, int dat_len)
{
    
    if (!strncmp(topic, ticos_command_request_topic, strlen(ticos_command_request_topic))) {
        ticos_command_receive(dat, dat_len);
    } else if (!strncmp(topic, ticos_property_desired_topic, strlen(ticos_property_desired_topic))) {
        ticos_property_receive(dat, dat_len);
    }else if (!strncmp(topic, ticos_system_user_response_topic, strlen(ticos_system_user_response_topic))) {
        ticos_system_user_response_receive(dat, dat_len);
    }else if (!strncmp(topic, ticos_ota_cloud_response, strlen(ticos_ota_cloud_response))) {
        ticos_ota_response(topic, topic_len, dat, dat_len);
    }
    
    
}



static ticos_event_cb_t m_ticos_evt_cb = NULL;
static void *m_ticos_user_data = NULL;
void set_ticos_event_cb(ticos_event_cb_t evt_cb, void *user_data)
{
    m_ticos_evt_cb = evt_cb;
    m_ticos_user_data = user_data;
}

void ticos_event_notify(ticos_evt_t evt)
{
    if (m_ticos_evt_cb)
        m_ticos_evt_cb(m_ticos_user_data, evt);
}
extern char *esp_version_get();
void ticos_ota_request(const char* productID, const char* deviceID, const char* currVer)
{
    char *up_data = (char *)malloc(512);
    if(up_data == NULL){
        //ESP_LOGE(TAG, "MQTT up_data malloc failed");
        return;
    }
    memset(up_data, 0, 512);
    snprintf(up_data, 512, "{\"path\":\"/ota/checkUpdate?deviceId=%s&productId=%s&currentVersion=%s\"}"\
    , deviceID, productID, esp_version_get());
    
    //ESP_LOGI(TAG, "ticloud_ota_start push topic:%s payload:%s", pub_topic, up_data);
    ticloud_ota_report_measure(OTA_MEASURE_PROCESSING);
    int ret = ticos_hal_mqtt_publish(ticos_ota_cloud_request, up_data, strlen(up_data), 1, 0);

    if (ret < 0) {
        //ESP_LOGE(TAG, "MQTT Publish failed");
        ticloud_ota_report_measure(OTA_MEASURE_FAIL);
        free(up_data);
        return;
    }
    free(up_data);
    return;
}

void ticloud_ota_report_measure(OTAMeasureStatus_t measure)
{
    ota_measure = measure;
}

void ticloud_ota_report_update(OTAUpdateStatus_t update)
{
    ota_update = update;
}

void ticloud_ota_report_progress(OTAUpdateStatus_t percent)
{
    progress = percent;
}
