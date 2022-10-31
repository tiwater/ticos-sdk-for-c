#include <ticos_api.h>
#include <WiFi.h>
#include <time.h>
#include <stdio.h>

#include "user_app.h"

#define SSID        "Tiwater"
#define PASSWORD    "Ti210223"
#define PRODUCT_ID  "BOB45WX7H4"
#define DEVICE_ID   "TEST002"
#define DEVICE_SECRET   "7rjQAIYU7DPULJo8YlppEg=="

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

static void connectToWiFi()
{
  printf("wifi connect start\r\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    printf("*\n");
  }
  printf("wifi connect ok\n");
}

static void initializeTime()
{
  printf("sntp time start\r\n");
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    now = time(nullptr);
    printf("T--\n");
  }
  printf("sntp end\r\n");
}

void setup()
{
  // 板级配置
  user_init();
  // 连接网络
  connectToWiFi();
  // 同步网络时间
  initializeTime();
  // 建立和 Ticos Cloud 的连接
  ticos_cloud_start(PRODUCT_ID, DEVICE_ID, DEVICE_SECRET);
}

void loop()
{
  // 扫描按键，处理应用的业务逻辑
  key_scan();
}
