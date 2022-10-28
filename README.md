# Ticos SDK 概述

Ticos SDK 提供了 Ticos Cloud 协议接入方案，SDK使用了 MQTT 协议用于和云端进行通信，支持开发者快速接入 WIFI 设备到 Ticos Cloud平台。
Ticos SDK 封装了协议实现细节和数据传输过程，让开发者可以聚焦在数据处理上，以达到快速开发的目的。


# 使用说明

## 安装 SDK

### Arduino

  1. Arduino IDE 安装
     - 在 Arduino IDE 中, 选择菜单 `项目`, `加载库`, `管理库...`。
     - 搜索并安装 `ticos-sdk-for-c`。 (当前库还未过审，请参考下面步骤手动安装)
  2. 手动安装
     - 将本 [Ticos SDK](https://github.com/tiwater/ticos-sdk-for-c) 克隆至 Arduino 库目录，通常该目录在 ～/Documents/Arduino/libraries，请根据你的开发平台中 Arduino IDE 的配置确定。

### 平台原生开发环境

  - 将本 [Ticos SDK](https://github.com/tiwater/ticos-sdk-for-c) 克隆至你工程开发环境，确保编译时包含本 SDK 的所有代码。

## 主要接口说明
  * API 接口: src/ticos_api.h

  - MCU在网络顺畅的情况下，调用提供 ticos_cloud_start() 启动云服务；
  - 连接成功后，物模型属性发生改变时，用户可主动调用 ticos_property_report() 上报属性到云端；
  - 云端下发数据时，需要调用 ticos_property_receive() 进行解析；
  - 用户可主动调用 ticos_cloud_stop() 结束云端的连接。

## SDK 集成

开发者集成本 SDK 接入 Ticos Cloud 需要做的工作有：

1. 在[河图](https://console.ticos.cn)中创建硬件产品，并根据产品需求定义出物模型；
   
2. 通过脚本将物模型文件转换为C代码，添加相应的数据处理：

   - 要求: 已安装 python3 运行环境；
   - 将服务端下载的物模型文件(例: thing_model.json)放到 tools/codegen 目录下；
   - 在 tools/codegen 目录下运行: python ./ticos_thingmodel_gen.py --json_file thing_model.json；
   - 成功后会在当前目录下产生 ticos_thingmodel.c 和 ticos_thingmodel.h 等文件, 将生成的文件移入用户工程中的源文件目录；
   - 在 ticos_thingmodel.c 中填入完成用户的业务逻辑。_send 后缀的函数为设备端向云端发送物模型对应属性/遥测时回调的接口，函数应返回该属性/遥测的值，通常是从物理设备获取到对应的值后返回，由 SDK 将该值上传至云端；_recv 后缀的函数为设备端接收到云下发的属性/命令时调用的接口，函数的参数即为接收到的值，用户根据业务需求对该值进行处理；

3. 提供对应硬件平台的 MQTT client 实现，使 SDK 可接入云端服务器，可参考 examples/Ticos_Iot_Hub_ESP32/ticos_mqtt_wrapper.cpp 相应的接口实现:

   - 提供 ticos_cloud_start() 函数，能启动平台相关的 MQTT client 客户端连接到 Ticos Cloud；
   - 提供 ticos_mqtt_client_publish() 函数，将数据上报到云端；
   - MQTT 连接成功后，订阅属性相关的 topic: "devices/{$deviceID}/twin/patch/desired"；
   - 提供函数 ticos_property_receive()，在 MQTT 接收到数据后进行数据的处理；
   - 根据河图中的产品定义信息，为 MQTT 连接提供 MQTT 服务器地址(`IOT_CONFIG_IOTHUB_FQDN`)、产品 ID (`IOT_CONFIG_PRODUCT_ID`)、设备 ID (`IOT_CONFIG_DEVICE_ID`) 这几组值。

执行以上步骤后，即完成了对 SDK 的集成工作，可以尝试编译运行你的项目，应可直接接入 Ticos Cloud 进行操作。

## 示例
   * 基于 ESP32 系列的工程示例: [Ticos IoT Hub ESPRESSIF ESP-32](examples/Ticos_IoT_Hub_ESP32/readme.md)。

### License

Ticos SDK for Embedded C is licensed under the [MIT](https://github.com/tiwater/ticos-sdk-for-c/blob/main/LICENSE) license.

