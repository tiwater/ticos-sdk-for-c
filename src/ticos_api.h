// Copyright (c) Tiwater Technology Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
/**
 * @file ticos_api.h
 * @brief Ticos API 平台接口定义
 *
 * Ticos 云平台 SDK 平台接口定义，用户应提供这些接口针对指定硬件平台的实现。
 *
 * @date 13 Nov 2022
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "ticos_thingmodel_type.h"
void ticos_cloud_set_bind_user_ID(char *token, int token_len);
int ticos_system_user_bind(void);
/**
 * @brief 启动 ticos 云服务.
 * @note  此接口需要用户实现，提供一个mqtt客户端服务，并启动连接到ticos cloud.
 * @param product_id 产品 ID
 * @param device_id 设备 ID
 * @param device_secret 设备密钥
 * @return 0 代表成功，其他值代表错误
 */
int ticos_cloud_start(const char* product_id, const char* device_id, const char *device_secret);

/**
 * @brief 停止 ticos 云服务.
 * @note  此接口需要用户实现，停止mqtt客户端与ticos cloud的连接.
 * @return void
 */
void ticos_cloud_stop();
int ticos_cloud_connected();
/**
 * @brief  上报物模型属性到云端
 * @note   此接口会上报用户在ti_thingmodel.c里面定义的属性值到云端
 * @return 0 代表成功，其他值代表错误
 */
int ticos_property_report(void);

/**
 * @brief  上报单个属性值到云端
 * @note   此接口会上报用户在ti_thingmodel.h里面定义的属性值到云端
 * @param index 该值定义在ticos_property_t中, 不能超过TICOS_PROPERTY_MAX，否则上传失败
 * @return 0 for success, other is error
 */
int ticos_property_report_by_index(int index);

/**
 * @brief  上报遥测到云端
 * @note   此接口会上报用户在ti_thingmodel.c里面定义的遥测到云端
 * @return 0 代表成功，其他值代表错误
 */
int ticos_telemetry_report(void);

/**
 * @brief  上报单个遥测值到云端
 * @note   此接口会上报用户在ti_thingmodel.h里面定义的遥测值到云端
 * @param index 该值定义在ticos_telemetry_t中, 不能超过TICOS_TELEMETRY_MAX，否则上传失败
 * @return 0 for success, other is error
 */
int ticos_telemetry_report_by_index(int index);

/**
 * @brief  订阅ticos cloud需要处理的topic
 * @note   此接口需要在mqtt客户端连接上的时候调用，监听云端下发的消息
 * @return 0 代表成功，其他值代表错误
 */
int ticos_mqtt_subscribe(void);

/**
 * @brief  云端下发数据数据解析
 * @note   此接口处理云端下发的数据，然后根据topic解析接收到的命令或属性
 * @param topic 接收到的topic
 * @param dat 接收到的数据指针
 * @param len 接收到的数据长度
 * @return void
 */
void ticos_msg_recv(const char *topic, int topic_len, const char *dat, int dat_len);

typedef enum {
    TICOS_EVENT_CONNECT,
    TICOS_EVENT_DISCONNECT,
} ticos_evt_t;

typedef void (*ticos_event_cb_t)(void *user_data, ticos_evt_t event);

/**
 * @brief  设置事件处理函数
 * @note   当有cloud相关的事件时, 会回调用户设置的事件函数
 * @param evt_cb 用户事件回调函数
 * @param user_data 用户数据
 * @return void
 */
void set_ticos_event_cb(ticos_event_cb_t evt_cb, void *user_data);

/**
 * @brief  云端事件通知函数
 * @note   当cloud产生相关的事件时, 会调用此函数
 * @param evt 相关事件
 * @return void
 */
void ticos_event_notify(ticos_evt_t evt);
void ticos_device_bind_cb(int bind_result);
void ticos_ota_request(const char* productID, const char* deviceID, const char* currVer);
void ticos_ota_response(const char *topic, int topic_len, const char *data, int data_len);
void ticloud_ota_report_measure(OTAMeasureStatus_t measure);
void ticloud_ota_report_update(OTAUpdateStatus_t update);
void ticloud_ota_report_progress(OTAUpdateStatus_t percent);
OTAMeasureStatus_t ota_measure_get();

OTAUpdateStatus_t ota_progress_get(int *ota_progress);
void ticloud_ota_finnish_callback_register( void (*cb)(void*) );
char *esp_version_get();
char *ticloud_ota_version_get();
void ticos_ota_start_update();
#ifdef __cplusplus
}
#endif
