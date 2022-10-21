## Introduction

This is a "to-the-point" guide outlining how to run an Ticos SDK IoT Hub sample on an ESP32 microcontroller.

## Prerequisites

- Have the latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) installed.

- Have the [ESP32 board support](https://github.com/espressif/arduino-esp32) installed on Arduino IDE.

    - ESP32 boards are not natively supported by Arduino IDE, so you need to add them manually.
    - Follow the [instructions](https://github.com/espressif/arduino-esp32) in the official ESP32 repository.

## Setup and Run Instructions

1. Run the Arduino IDE.

2. Install the Ticos SDK for Embedded C library.

    - On the Arduino IDE, go to menu `Sketch`, `Include Library`, `Manage Libraries...`.
    - Search for and install `ticos-sdk-for-c`. (not valid current, please contact Tiwater Technology Co. Ltd)

3. Open the ESPRESSIF ESP32 sample.

    - On the Arduino IDE, go to menu `File`, `Examples`, `ticos-sdk-for-c`.
    - Click on `Ticos_Iot_Hub_ESP32` to open the sample.

4. Configure the ESPRESSIF ESP32 sample.

    Enter your Ticos IoT Hub and device information into the file `ti_iot_hal.c`:
    - Add you IoTHub Name to `IOT_CONFIG_IOTHUB_FQDN`
    - Add your product ID to `IOT_CONFIG_PRODUCT_ID`
    - Add your Device ID to `IOT_CONFIG_DEVICE_ID`

    Enter your Wi-Fi information into the file `Ticos_Iot_Hub_ESP32.ino`:
    - Add your Wi-Fi SSID to `IOT_CONFIG_WIFI_SSID`
    - Add your Wi-Fi password to `IOT_CONFIG_WIFI_PASSWORD`

5. Connect the ESP32 microcontroller to your USB port.

6. On the Arduino IDE, select the board and port.

    - Go to menu `Tools`, `Board` and select `ESP32`.
    - Go to menu `Tools`, `Port` and select the port to which the microcontroller is connected.

7. Upload the sketch.

    - Go to menu `Sketch` and click on `Upload`.

        <details><summary><i>Expected output of the upload:</i></summary>
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

8. Monitor the MCU (microcontroller) locally via the Serial Port.

    - Go to menu `Tools`, `Serial Monitor`.

        If you perform this step right away after uploading the sketch, the serial monitor will show an output similar to the following upon success:

        ```text
        Connecting to WIFI SSID buckaroo
        .......................WiFi connected, IP address:
        192.168.1.123
        Setting time using SNTP..............................done!
        Current time: Thu May 28 02:55:05 2020
        Client ID: mydeviceid
        Username: myiothub.ticos-devices.net/mydeviceid/?api-version=2018-06-30&DeviceClientType=c%2F1.0.0
        SharedAccessSignature sr=myiothub.ticos-devices.net%2Fdevices%2Fmydeviceid&sig=placeholder-password&se=1590620105
        MQTT connecting ... connected.
        ```

### License

Ticos SDK for Embedded C is licensed under the [MIT](https://github.com/tiwater/ticos-sdk-for-c/blob/main/LICENSE) license.
