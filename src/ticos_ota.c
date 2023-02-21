#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "ticos_api.h"
#include "ticos_esp_storage.h"
//#include "ticos_esp_prov.h"
esp_ticos_ota_info_t ticos_ota_info = {0};
OTAMeasureStatus_t  ota_measure = OTA_MEASURE_IDLE;
OTAUpdateStatus_t   ota_update = OTA_UPDATE_IDLE;
uint8_t progress = 0;
extern char ticos_ota_cloud_request[128];
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

void ticos_ota_response(const char *topic, int topic_len, const char *data, int data_len)
{
    if (strncmp(data, "{}", data_len) == 0) {
        ticloud_ota_report_measure(OTA_MEASURE_SUCCESS_NO_UPDATE);
        return;
    }
    cJSON *response  = cJSON_Parse(data);
    if(response == NULL){
        ticloud_ota_report_measure(OTA_MEASURE_FAIL);
        //ESP_LOGE(TAG, "response data is not a json");
        return;
    } 

    const char *version = cJSON_GetObjectItem(response, "targetVersion")->valuestring;
    const char *file_url = cJSON_GetObjectItem(response, "fileUrl")->valuestring;
    const char *md5 = cJSON_GetObjectItem(response, "fileSign")->valuestring;
    uint32_t file_size = cJSON_GetObjectItem(response, "fileSize")->valueint;
    if(version == NULL || file_url == NULL || file_size == 0 || md5 == NULL){
        ticloud_ota_report_measure(OTA_MEASURE_FAIL);
        //ESP_LOGE(TAG, "response data node is invaild");
    }else{
        ticos_ota_info.file_size = file_size;
        strcpy(ticos_ota_info.md5sum, md5);
        strcpy(ticos_ota_info.url, file_url);
        strcpy(ticos_ota_info.version, version);
        //ESP_LOGI(TAG, "md5:%s url:%s version:%s size:%d", md5, file_url, version, file_size);
        if(strcmp(version, esp_version_get()))
        {
            ticos_esp_storage_get( "ota_varsion" , version , 32 );
            ticloud_ota_report_measure(OTA_MEASURE_SUCCESS_NEED_UPDATE);
        }else{
            ticloud_ota_report_measure(OTA_MEASURE_SUCCESS_NO_UPDATE);
        }
    }
    //}
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
