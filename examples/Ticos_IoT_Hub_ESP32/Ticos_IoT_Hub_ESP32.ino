#include "my_mqtt.h"
#include "user_app.h"
#include "WiFi.h"
#include "time.h"
#include <ti_iot_api.h>

#define IOT_CONFIG_WIFI_SSID              "WIFI_SSID"
#define IOT_CONFIG_WIFI_PASSWORD          "WIFI_PWD"
static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;

#define IOT_CONFIG_IOTHUB_FQDN            "hub.ticos.cn"
#define IOT_CONFIG_DEVICE_ID              "SLC1"
#define IOT_CONFIG_PRODUCT_ID             "HITXM3K4IE"

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

int ti_iot_mqtt_client_publish(const char* topic, const char* data, size_t len)
{
    return my_mqtt_client_publish(topic, data, len) ? 0 : 1;
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
  ti_iot_client_init(IOT_CONFIG_IOTHUB_FQDN,
                     IOT_CONFIG_PRODUCT_ID,
                     IOT_CONFIG_DEVICE_ID);
  my_mqtt_client_set_on_recv_cb(ti_iot_property_receive);
  my_mqtt_client_start(IOT_CONFIG_IOTHUB_FQDN,
                       ti_iot_mqtt_client_id(),
                       ti_iot_mqtt_username(),
                       IOT_CONFIG_DEVICE_ID);
}

void loop()
{
  // 扫描按键，处理应用的业务逻辑
  key_scan();
}
