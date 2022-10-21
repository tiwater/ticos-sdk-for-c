## 概述

ticos-sdk-for-c 提供了 ticos cloud 协议接入方案，SDK使用了 mqtt 协议用于和云端进行通信，支持开发者快速接入WIFI设备到ticos cloud平台。
ticos-sdk-for-c 封装了协议实现细节和数据传输过程，让开发者可以聚焦在数据处理上，以达到快速开发的目的。


## 软件移植

开发者接入 ticos cloud 需要做的工作有：

1. 提供mqtt client接入云端服务器，参考examples/Ticos_Iot_Hub_ESP32/ti_iot_hal.cpp完成相应的接口
2. 通过脚本将物模型文件转换为C代码，添加相应的数据处理


## 使用说明

* 参考 src/ti_iot_api.h

