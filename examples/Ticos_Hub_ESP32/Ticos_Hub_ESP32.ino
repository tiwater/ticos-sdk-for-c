#include <ticos_api.h>
#include <WiFi.h>
#include <time.h>

#include "user_app.h"

#define SSID        "WIFI_SSID"
#define PASSWORD    "WIFI_PSWD"
#define PRODUCT_ID  "HITXM3K4IE"
#define DEVICE_ID   "SLC1"
#define DEVICE_SECRET   "7rjQAIYU7DPULJo8YlppEg=="

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

static void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

static void initializeTime()
{
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    now = time(nullptr);
  }
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
