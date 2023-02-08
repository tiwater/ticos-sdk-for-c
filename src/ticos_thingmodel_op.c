#include "ticos_thingmodel_type.h"
#include <string.h>
#include "cJSON.h"
#include "esp_qcloud_prov.h"
#include "ticos_api.h"
#include "esp_qcloud_storage.h"
//#include "ticos_thingmodel.h"
extern const ticos_telemetry_info_t ticos_telemetry_tab[];
extern const ticos_property_info_t ticos_property_tab[];
extern const ticos_command_info_t ticos_command_tab[];
extern const int ticos_telemetry_cnt;
extern const int ticos_property_cnt;
extern const int ticos_command_cnt;

extern char ticos_property_report_topic[];
extern char ticos_telemery_topic[];
extern char ticos_system_user_id[];
extern char ticos_system_user_bind_topic[];
typedef int (*_ticos_send_int_t)();
typedef int (*_ticos_send_bool_t)();
typedef float (*_ticos_send_float_t)();
typedef const char* (*_ticos_send_string_t)();

typedef void (*_ticos_recv_int_t)(int);
typedef void (*_ticos_recv_bool_t)(int);
typedef void (*_ticos_recv_float_t)(float);
typedef void (*_ticos_recv_string_t)(const char*);



int ticos_hal_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);

int ticos_telemetry_report(void)
{
    cJSON *telemetries = cJSON_CreateObject();

    for (int i = 0; i < ticos_telemetry_cnt; i++) {
        void *func = ticos_telemetry_tab[i].func;
        switch (ticos_telemetry_tab[i].type) {
        case TICOS_VAL_TYPE_BOOLEAN:
            cJSON_AddBoolToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_bool_t)func)());
            break;
        case TICOS_VAL_TYPE_INTEGER:
            cJSON_AddNumberToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_int_t)func)());
            break;
        case TICOS_VAL_TYPE_FLOAT:
            cJSON_AddNumberToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_float_t)func)());
            break;
        case TICOS_VAL_TYPE_STRING:
            cJSON_AddStringToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_string_t)func)());
            break;
        default:
            cJSON_AddNullToObject(telemetries, ticos_telemetry_tab[i].id);
            break;
        }
    }

    char *telemetries_str = cJSON_PrintUnformatted(telemetries);

    int ret = ticos_hal_mqtt_publish(ticos_telemery_topic, telemetries_str, strlen(telemetries_str), 1, 0);
    cJSON_free(telemetries_str);
    cJSON_Delete(telemetries);
    return ret;
}

void ticos_command_receive(const char *dat, int len)
{
    cJSON *commands = cJSON_Parse(dat);
    if ((!commands) || (!cJSON_IsObject(commands)))
        return;

    int size = cJSON_GetArraySize(commands);
    for (int i = 0; i < size; i++) {
        cJSON *command = cJSON_GetArrayItem(commands, i);
        for (int j = 0; j < ticos_command_cnt; j++) {
            if (!strcmp(command->string, ticos_command_tab[j].id)) {
                void *command_func = ticos_command_tab[j].func;
                switch (ticos_command_tab[j].type) {
                case TICOS_VAL_TYPE_BOOLEAN:
                    if (cJSON_IsBool(command))
                        ((_ticos_recv_bool_t)command_func)(cJSON_IsTrue(command));
                    break;
                case TICOS_VAL_TYPE_INTEGER:
                    if (cJSON_IsNumber(command))
                        ((_ticos_recv_int_t)command_func)(cJSON_GetNumberValue(command));
                    break;
                case TICOS_VAL_TYPE_FLOAT:
                    if (cJSON_IsNumber(command))
                        ((_ticos_recv_float_t)command_func)(cJSON_GetNumberValue(command));
                    break;
                case TICOS_VAL_TYPE_STRING:
                    if (cJSON_IsString(command))
                        ((_ticos_recv_string_t)command_func)(cJSON_GetStringValue(command));
                    break;
                default:
                    break;
                }
            }
        }
    }
    cJSON_Delete(commands);
}

void ticos_system_user_response_receive(const char *dat, int len)
{
    ticos_device_bind_cb(1);
    esp_qcloud_prov_ble_report_bind_status(0);
    vTaskDelay(10);
    esp_qcloud_prov_ble_stop();
    
}

void ticos_property_receive(const char *dat, int len)
{
    cJSON *propretys = cJSON_Parse(dat);
    if ((!propretys) || (!cJSON_IsObject(propretys)))
        return;

    int size = cJSON_GetArraySize(propretys);
    for (int i = 0; i < size; i++) {
        cJSON *property = cJSON_GetArrayItem(propretys, i);
        for (int j = 0; j < ticos_property_cnt; j++) {
            if (!strcmp(ticos_property_tab[j].id, property->string)) {
                void *recv_func = ticos_property_tab[j].recv_func;
                switch (ticos_property_tab[j].type) {
                case TICOS_VAL_TYPE_BOOLEAN:
                    if (cJSON_IsBool(property))
                        ((_ticos_recv_bool_t)recv_func)(cJSON_IsTrue(property));
                    break;
                case TICOS_VAL_TYPE_INTEGER:
                    if (cJSON_IsNumber(property))
                        ((_ticos_recv_int_t)recv_func)(cJSON_GetNumberValue(property));
                    break;
                case TICOS_VAL_TYPE_FLOAT:
                    if (cJSON_IsNumber(property))
                        ((_ticos_recv_float_t)recv_func)(cJSON_GetNumberValue(property));
                    break;
                case TICOS_VAL_TYPE_STRING:
                    if (cJSON_IsString(property))
                        ((_ticos_recv_string_t)recv_func)(cJSON_GetStringValue(property));
                    break;
                default:
                    break;
                }
                break;
            }
        }
    }
    cJSON_Delete(propretys);
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
int ticos_property_report(void)
{
    cJSON *propretys = cJSON_CreateObject();

    for (int i = 0; i < ticos_property_cnt; i++) {
        void *func = ticos_property_tab[i].send_func;
        switch (ticos_property_tab[i].type) {
        case TICOS_VAL_TYPE_BOOLEAN:
            cJSON_AddBoolToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_bool_t)func)());
            break;
        case TICOS_VAL_TYPE_INTEGER:
            cJSON_AddNumberToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_int_t)func)());
            break;
        case TICOS_VAL_TYPE_FLOAT:
            cJSON_AddNumberToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_float_t)func)());
            break;
        case TICOS_VAL_TYPE_STRING:
            cJSON_AddStringToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_string_t)func)());
            break;
        default:
            cJSON_AddNullToObject(propretys, ticos_property_tab[i].id);
            break;
        }
    }

    char *propertys_str = cJSON_PrintUnformatted(propretys);
    int ret = ticos_hal_mqtt_publish(ticos_property_report_topic, propertys_str, strlen(propertys_str), 1, 0);
    cJSON_free(propertys_str);
    cJSON_Delete(propretys);
    return ret;
}

int ticos_property_report_by_index(int index)
{
    if (index >= ticos_property_cnt)
        return -1;

    int i = index;
    cJSON *propretys = cJSON_CreateObject();
    void *func = ticos_property_tab[i].send_func;
    switch (ticos_property_tab[i].type) {
    case TICOS_VAL_TYPE_BOOLEAN:
        cJSON_AddBoolToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_bool_t)func)());
        break;
    case TICOS_VAL_TYPE_INTEGER:
        cJSON_AddNumberToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_int_t)func)());
        break;
    case TICOS_VAL_TYPE_FLOAT:
        cJSON_AddNumberToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_float_t)func)());
        break;
    case TICOS_VAL_TYPE_STRING:
        cJSON_AddStringToObject(propretys, ticos_property_tab[i].id, ((_ticos_send_string_t)func)());
        break;
    default:
        cJSON_AddNullToObject(propretys, ticos_property_tab[i].id);
        break;
    }
    char *propertys_str = cJSON_PrintUnformatted(propretys);
    int ret = ticos_hal_mqtt_publish(ticos_property_report_topic, propertys_str, strlen(propertys_str), 1, 0);
    cJSON_free(propertys_str);
    cJSON_Delete(propretys);
    return ret;
}

int ticos_telemetry_report_by_index(int index)
{
    if (index >= ticos_telemetry_cnt)
        return -1;

    int i = index;
    cJSON *telemetries = cJSON_CreateObject();
    void *func = ticos_telemetry_tab[i].func;
    switch (ticos_telemetry_tab[i].type) {
    case TICOS_VAL_TYPE_BOOLEAN:
        cJSON_AddBoolToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_bool_t)func)());
        break;
    case TICOS_VAL_TYPE_INTEGER:
        cJSON_AddNumberToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_int_t)func)());
        break;
    case TICOS_VAL_TYPE_FLOAT:
        cJSON_AddNumberToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_float_t)func)());
        break;
    case TICOS_VAL_TYPE_STRING:
        cJSON_AddStringToObject(telemetries, ticos_telemetry_tab[i].id, ((_ticos_send_string_t)func)());
        break;
    default:
        cJSON_AddNullToObject(telemetries, ticos_telemetry_tab[i].id);
        break;
    }
    char *telemetries_str = cJSON_PrintUnformatted(telemetries);
    int ret = ticos_hal_mqtt_publish(ticos_telemery_topic, telemetries_str, strlen(telemetries_str), 1, 0);
    cJSON_free(telemetries_str);
    cJSON_Delete(telemetries);
    return ret;
}

extern esp_ticos_ota_info_t ticos_ota_info;
extern char *esp_version_get();
void ticos_ota_response(const char *topic, int topic_len, const char *data, int data_len)
{
    //if (strncmp(topic, ticos_ota_cloud_response, topic_len) == 0) {
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
                esp_qcloud_storage_get( "ota_varsion" , version , 32 );
                ticloud_ota_report_measure(OTA_MEASURE_SUCCESS_NEED_UPDATE);
            }else{
                ticloud_ota_report_measure(OTA_MEASURE_SUCCESS_NO_UPDATE);
            }
        }
    //}
}






