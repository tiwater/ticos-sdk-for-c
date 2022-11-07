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
void ticos_msg_recv(const char *topic, const char *dat, int len);


#ifdef __cplusplus
}
#endif
