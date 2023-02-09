#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <time.h>

typedef enum {
    TICOS_VAL_TYPE_BOOLEAN,
    TICOS_VAL_TYPE_INTEGER,
    TICOS_VAL_TYPE_DOUBLE,//临时增加，生成未定义
    TICOS_VAL_TYPE_FLOAT,
    TICOS_VAL_TYPE_STRING,
    TICOS_VAL_TYPE_ENUM,
    TICOS_VAL_TYPE_TIMESTAMP,
    TICOS_VAL_TYPE_DURATION,
    TICOS_VAL_TYPE_MAX,
} ticos_val_type_t;

typedef struct {
    time_t start;
    time_t end;
} ticos_val_duration_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *func;
} ticos_telemetry_info_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *send_func;
    void *recv_func;
} ticos_property_info_t;

typedef struct {
    const char *id;
    ticos_val_type_t type;
    void *func;
} ticos_command_info_t;

typedef struct {
    size_t file_size;
    char md5sum[33];
    char url[512];
    char version[32];
    uint32_t start_timetemp;
    size_t download_size;
    uint8_t download_percent;
} esp_ticos_ota_info_t;
typedef enum {
    OTA_MEASURE_IDLE = 0,
    OTA_MEASURE_PROCESSING,
    OTA_MEASURE_FAIL,
    OTA_MEASURE_SUCCESS_NO_UPDATE,
    OTA_MEASURE_SUCCESS_NEED_UPDATE,
} OTAMeasureStatus_t;

typedef enum {
    OTA_UPDATE_IDLE = 0,
    OTA_UPDATE_DOWNLOAD,
    OTA_UPDATE_FLASH,
    OTA_UPDATE_FAIL,
    OTA_UPDATE_SUCCESS,
} OTAUpdateStatus_t;
typedef struct ticos_skin_res{
    //char acid[40];
    //char userid[40];
    int skinscore;
    //int monthtimes;
    //char imageurl[200];
    //char reportUrl[200];
    int status;         // 1. 检测中.  2 检测完成            3 检测失败.
    //char createdtime[40];
    char summary[200];
    //char productid[20];
    //char devicename[20];
}ticos_skin_res_t;
#ifdef __cplusplus
}
#endif
