// Copyright (c) Tiwater Technology Ltd. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief 本头文件定义了用户通过 Ticos SDK 接入 Ticos Cloud 时需要用到的接口
 *
 * @note 你不能使用任何带有 '_' 前缀的标记（宏，函数，结构体，枚举等）。这些标记是 Ticos SDK 的
 * 内部实现；我们没有为这些标记提供文档说明，它们很可能在 SDK 今后的版本中发生改变从而导致你的程序无法运行。
 */

#pragma once

#include <_ti_cfg_prefix.h>

/**
 * @brief  上报物模型属性到云端
 * @note   此接口会上报用户在ti_thingmodel.c里面定义的属性值到云端
 * @return TI_OK for success, other is error
 */
int ti_iot_property_report(void);

/**
 * @brief  云端下发属性数据解析
 * @note   此接口会解析云端下发的属性数据，然后回调用户在ti_thingmodel.c里面定义的属性处理函数
 * @param dat 接收到的数据指针
 * @param len 接收到的数据长度
 * @return void
 */
void ti_iot_property_receive(const char *dat, int len);

/**
 * @brief  云端下发命令数据解析
 * @note   此接口会解析云端下发的命令数据，然后回调用户在ti_thingmodel.c里定义的命令处理函数
 * @param dat 接收到的数据指针
 * @param len 接收到的数据长度
 * @return void
 */
void ti_iot_command_receive(const char *dat, int len);

/**
 * @brief 初始化 ti_iot_client
 * @note  此函数必须先于任何其他 ti_iot 族函数被开发者调用
 * @param mqtt_fqdn  mqtt hub 地址
 * @param product id 产品 ID
 * @param device id  设备 ID
 * @return true if success, else fail
 */
bool ti_iot_client_init(const char* mqtt_fqdn,
                            const char* product_id,
                            const char* device_id);

/**
  * @brief 获取 mqtt_client_id
  * @return mqtt_client_id
  */
const char* ti_iot_mqtt_client_id(void);

/**
  * @brief 获取 mqtt_username
  * @return mqtt_username
  */
const char* ti_iot_mqtt_username(void);

/**
 * @brief MQTT 客户端向云端推送数据的接口
 * @note  Ticos SDK 会调用此接口，完成数据的上传。需要用户实现此函数
 * @param topic 上报信息的topic
 * @param data 上报的数据内容
 * @param len  上报的数据长度
 * @return TI_OK for success, other for fail.
 */
int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len);

#include <_ti_cfg_suffix.h>
