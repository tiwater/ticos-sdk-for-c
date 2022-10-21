## 概述

ticos-sdk-for-c 提供了 ticos cloud 协议接入方案，SDK使用了 mqtt 协议用于和云端进行通信，支持开发者快速接入WIFI设备到ticos cloud平台。
ticos-sdk-for-c 封装了协议实现细节和数据传输过程，让开发者可以聚焦在数据处理上，以达到快速开发的目的。


## 使用说明

* 基于esp32系列的工程示例: examples/Ticos_Iot_Hub_ESP32
* api接口: src/ti_iot_api.h

- MCU在网络顺畅的情况下，调用提供ti_iot_cloud_start()启动云服务
- 连接成功后，用户可主动调用ti_iot_property_report()上报属性到云端
- 云端下发数据时，需要调用ti_iot_property_receive()进行解析
- 用户可主动调用ti_iot_cloud_stop()结束云端的连接
## 软件移植

开发者接入 ticos cloud 需要做的工作有：

1. 提供mqtt client接入云端服务器，参考examples/Ticos_Iot_Hub_ESP32/ti_iot_hal.cpp相应的接口:

- 提供ti_iot_cloud_start()函数，能启动平台相关的mqtt client客户端连接到ticos cloud
- 提供ti_iot_mqtt_client_publish()函数，将数据上报到云端
- 提供ti_iot_get_device_id()函数，获取设备deviceID
- mqtt连接成功后，订阅属性相关的topic: "devices/{$deviceID}/twin/patch/desired"
- mqtt接收数据后，调用sdk中ti_iot_property_receive函数进行数据的处理


2. 通过脚本将物模型文件转换为C代码，添加相应的数据处理

- 要求: 已安装python2或python3运行环境
- 将服务端下载的物模型文件(例: thing_model.json)放到tools/codegen目录下
- 在tools/codegen目录下运行: ./ticos_thingmodel_gen.py --json thing_model.json
- 成功后会在当前目录下产生ti_thingmodel.c和ti_thingmodel.h文件, 将生成的文件放入工程中编译
- 在ti_thingmodel.c中填入完成用户的业务逻辑
