#include "ticos_api.h"
#include "user_app.h"
#include "WiFi.h"
#include "time.h"

#define IOT_CONFIG_WIFI_SSID              "WIFI_SSID"
#define IOT_CONFIG_WIFI_PASSWORD          "WIFI_PWD"
static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF   1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

static void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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
  ticos_cloud_start();
}

void loop()
{
  // 扫描按键，处理应用的业务逻辑
  key_scan();
}
