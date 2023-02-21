
#include <string.h>
#include "cJSON.h"
#include "user_app_t3.h"
#include <esp_log.h>
#include "ticos_esp_prov.h"
#include "ticos_api.h"
#include <esp_ota_ops.h>
#include <esp_https_ota.h>
#include <esp_partition.h>
#include "ticos_esp_storage.h"
#include "ticos_export.h"
#define TAG     "user_t3"
static ticos_skin_res_t g_skin_res;
ticos_skin_res_t *ticos_get_skin_res(void)
{
    return &g_skin_res;
};
void ticos_data_response(const char *topic, int topic_len, const char *data, int data_len)
{
    
    cJSON *root_json  = cJSON_Parse(data);
    #define MIN(a,b) (((a)<(b))?(a):(b))
    if(!root_json){
         cJSON_Delete(root_json);
         return;
    }

    int skinscore = cJSON_GetObjectItem(root_json, "skinScore")->valueint;
    int status = cJSON_GetObjectItem(root_json, "status")->valueint;
    char *summary = cJSON_GetObjectItem(root_json, "summary")->valuestring;
    if(!summary){
        cJSON_Delete(root_json);
        return;
    }

    g_skin_res.skinscore = skinscore;
    g_skin_res.status = status;
    memset (g_skin_res.summary, 0x00, sizeof (g_skin_res.summary));
    memcpy (g_skin_res.summary, summary, MIN(strlen(summary), sizeof (g_skin_res.summary)));

}
char esp_version[32] = {0};
char *esp_version_get()
{
    if(esp_version[0] != 0) return esp_version;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;

    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        strncpy(esp_version, running_app_info.version, 32);
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }else{
        ESP_LOGE(TAG, "Get running_app_info fail");
    }
    return esp_version;
}

int ticos_system_user_bind(void)
{
    cJSON *user_bind = cJSON_CreateObject();
    cJSON_AddStringToObject(user_bind, "userId", ticos_system_user_id);
    char *user_bind_str = cJSON_PrintUnformatted(user_bind);
    int ret = ticos_hal_mqtt_publish(ticos_system_user_bind_topic, user_bind_str, strlen(user_bind_str), 1, 0);
    cJSON_free(user_bind_str);
    cJSON_Delete(user_bind);
    return ret;

}
void ticos_system_user_response_receive(const char *dat, int len)
{
    ticos_device_bind_cb(1);
    ticos_esp_prov_ble_report_bind_status(0);
    vTaskDelay(10);
    ticos_esp_prov_ble_stop();
    
}
extern OTAMeasureStatus_t  ota_measure;


OTAMeasureStatus_t ota_measure_get()
{
    if( ota_measure == OTA_MEASURE_IDLE){
        char last_ota_version[32];
        if( ticos_esp_storage_get( "ota_varsion" , last_ota_version , 32 ) == ESP_OK ){
            if( strcmp( last_ota_version , esp_version_get() ) != 0 ){
                return OTA_MEASURE_SUCCESS_NEED_UPDATE;
            }
        }
    }
    return ota_measure;
}
extern uint8_t progress;
extern OTAUpdateStatus_t   ota_update;
OTAUpdateStatus_t ota_progress_get(int *ota_progress)
{
    *ota_progress = progress;
    return ota_update;
}
