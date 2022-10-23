## Ticos Iot Hub Arduino ESP32 示例简介

本例演示如何编译并运行一个基于 ESP32 硬件平台和 Arduino 框架的 Ticos SDK IoT Hub 的示例程序。
本示例可通过开发板上的按键或云端/小程序控制开发板上的 LED 亮灭。

## 前提条件

- 已安装最新的 [Arduino IDE](https://www.arduino.cc/en/Main/Software)。
- 在 Arduino IDE 上已安装 [ESP32 板级支持包](https://github.com/espressif/arduino-esp32)。
    - ESP32 并不是 Arduino IDE 原生支持的，你需要额外安装。
    - 你也可以安装 Ticos 版的 [ESP32 板级支持包](https://github.com/tiwater/arduino-esp32)，以更好地适配 [Ticos Kit 开发套件](https://www.tiwater.com/ticos/kit/)。
        - 安装方法：打开 Arduino IDE 菜单：`Arduino`，`Preferences...`，将 `https://assets.ticos.cc/tiwater/package_ticos_kit_index.json` 添加至`附加开发板管理器网址`，保存选项并等待 Arduino IDE 更新完成后，即可在开发板中选择和 Ticos Kit 对应的开发板。 
- 在[河图](https://console.ticos.cn)中创建硬件产品，并根据产品需求定义出物模型：
    - 本示例假设该产品具有一个只读的 `switch` 属性，类型为 boolean，用于反映设备上轻触开关的状态；
    - 本示例假设该产品具有一个可读写的 `led` 属性，类型为 boolean，用于反映及控制设备上 LED 灯的状态。

## 配置及运行步骤

1. 运行 Arduino IDE。
2. 安装 SDK （如果你已经参考 SDK 中的说明完成安装，请忽略此步骤）：
   1. Arduino IDE 中安装
      - 在 Arduino IDE 中, 选择菜单 `项目`, `加载库`, `管理库...`。
      - 搜索并安装 `ticos-sdk-for-c`。 (当前库还未过审，请参考下面步骤手动安装)
   2. 手动安装
      - 将本 [Ticos SDK](https://github.com/tiwater/ticos-sdk-for-c-arduino) 克隆至 Arduino 库目录，通常该目录在 ～/Documents/Arduino/libraries，请根据你的开发平台中 Arduino IDE 的配置确定。
3. 打开示例：

    - 从 Arduino IDE 中打开菜单 `文件`, `示例`, `Ticos SDK for C`。
    - 点击 `Ticos_Iot_Hub_ESP32`，打开示例工程。

4. 配置示例：

    在文件 `ti_iot_hal.c` 中配置你的 Ticos IoT Hub 以及设备信息：
    - 设置 `IOT_CONFIG_IOTHUB_FQDN` 为你的 Ticos IoT Hub 域名地址；
    - 设置 `IOT_CONFIG_PRODUCT_ID` 为你的产品 ID；
    - 设置 `IOT_CONFIG_DEVICE_ID` 为你测试的设备 ID；
  
    在 `Ticos_Iot_Hub_ESP32.ino` 中输入你的 Wi-Fi 信息：
    - 设置 `IOT_CONFIG_WIFI_SSID` 为你的 Wi-Fi 名称；
    - 设置 `IOT_CONFIG_WIFI_PASSWORD` 为你的 Wi-Fi 密码；
  
    在 `user_app.c` 中根据你的外设所连接到的 GPIO 端口进行配置：
    - 设置 `KEY_GPIO` 为你按键 GPIO 端口；
    - 设置 `LED_GPIO` 为你的 LED 所连接到的 GPIO 端口。
  
    如果你的开发板暂时还没有连接到合适的外部设备，你也可以暂时先忽略这一步，这并不影响你的程序编译运行。等你做好了外设连接，你可以回过头来再进行配置。

5. 将你的 ESP32 开发板连接至开发电脑的 USB 口。

6. 在 Arduino IDE 中选择开发板、端口及其他配置：

    - 在菜单`工具`，`开发板`中选择你所持开发板的对应型号；
    - 在菜单`工具`，`端口`中选择你的开发板所连接至的端口；
    - `工具`菜单下的其他开发板配置项，请根据你的开发板硬件说明选择相应的配置。

7. 上传项目：

    - 在`项目`菜单中选择`上传`：

        <details><summary><i>可能的输出（随硬件及环境不同会有所变化）：</i></summary>
        <p>

        ```text
        Executable segment sizes:
        IROM   : 361788          - code in flash         (default or ICACHE_FLASH_ATTR)
        IRAM   : 26972   / 32768 - code in IRAM          (ICACHE_RAM_ATTR, ISRs...)
        DATA   : 1360  )         - initialized variables (global, static) in RAM/HEAP
        RODATA : 2152  ) / 81920 - constants             (global, static) in RAM/HEAP
        BSS    : 26528 )         - zeroed variables      (global, static) in RAM/HEAP
        Sketch uses 392272 bytes (37%) of program storage space. Maximum is 1044464 bytes.
        Global variables use 30040 bytes (36%) of dynamic memory, leaving 51880 bytes for local variables. Maximum is 81920 bytes.
        /home/user/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 /home/user/.arduino15/packages/esp8266/hardware/esp8266/2.7.1/tools/upload.py --chip esp8266 --port /dev/ttyUSB0 --baud 230400 --before default_reset --after hard_reset write_flash 0x0 /tmp/arduino_build_826987/ticos_iot_hub_telemetry.ino.bin
        esptool.py v2.8
        Serial port /dev/ttyUSB0
        Connecting....
        Chip is ESP8266EX
        Features: WiFi
        Crystal is 26MHz
        MAC: dc:4f:22:5e:a7:09
        Uploading stub...
        Running stub...
        Stub running...
        Changing baud rate to 230400
        Changed.
        Configuring flash size...
        Auto-detected Flash size: 4MB
        Compressed 396432 bytes to 292339...

        Writing at 0x00000000... (5 %)
        Writing at 0x00004000... (11 %)
        Writing at 0x00008000... (16 %)
        Writing at 0x0000c000... (22 %)
        Writing at 0x00010000... (27 %)
        Writing at 0x00014000... (33 %)
        Writing at 0x00018000... (38 %)
        Writing at 0x0001c000... (44 %)
        Writing at 0x00020000... (50 %)
        Writing at 0x00024000... (55 %)
        Writing at 0x00028000... (61 %)
        Writing at 0x0002c000... (66 %)
        Writing at 0x00030000... (72 %)
        Writing at 0x00034000... (77 %)
        Writing at 0x00038000... (83 %)
        Writing at 0x0003c000... (88 %)
        Writing at 0x00040000... (94 %)
        Writing at 0x00044000... (100 %)
        Wrote 396432 bytes (292339 compressed) at 0x00000000 in 13.0 seconds (effective 243.4 kbit/s)...
        Hash of data verified.

        Leaving...
        Hard resetting via RTS pin...
        ```

        </p>
        </details>

8. 通过串口监视单片机的输出：

    - 从菜单中选择`工具`，`串口监视器`：

        如果你上传后立即执行了这个动作，在连接成功的情况下，串口监视器可能会输出下列信息：

        ```[  1063][I][esp32-hal-psram.c:96] psramInit(): PSRAM enabled
            [  1089][D][WiFiGeneric.cpp:929] _eventCallback(): Arduino Event: 0 - WIFI_READY
            [  1177][D][WiFiGeneric.cpp:929] _eventCallback(): Arduino Event: 2 - STA_START
            [  1209][D][WiFiGeneric.cpp:929] _eventCallback(): Arduino Event: 4 - STA_CONNECTED
            [  1241][D][WiFiGeneric.cpp:929] _eventCallback(): Arduino Event: 7 - STA_GOT_IP
            [  1242][D][WiFiGeneric.cpp:991] _eventCallback(): STA IP: 192.168.0.137, MASK: 255.255.255.0, GW: 192.168.0.1
            MQTT client started
            MQTT event MQTT_EVENT_BEFORE_CONNECT
            MQTT event MQTT_EVENT_CONNECTED
            Subscribed for cloud-to-device messages
        ```
9. 测试：
    - 在[河图](https://console.ticos.cn)中进入`设备管理`，选择你正在测试的设备，进入`详情`页面，选择`数字孪生`，观察设备状态；
    - 按动开发板上的按钮，观察 LED 的亮灭，并刷新`详情`页面，观察云端的状态更新；
    - 在[河图](https://console.ticos.cn)中进入`设备管理`，选择你正在测试的设备，进入`调试`页面，设置和 `led` 对应的属性，并按下`下发`按钮，注意观察开发板上 LED 的亮灭。

### License

Ticos SDK for Embedded C is licensed under the [MIT](https://github.com/tiwater/ticos-sdk-for-c/blob/main/LICENSE) license.
