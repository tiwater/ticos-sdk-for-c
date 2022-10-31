#include <stdio.h>
#include <string.h>

int ticos_hal_mqtt_start(const char *url, int port, const char *client_id, const char *user_name, const char *passwd);
void ticos_hal_mqtt_stop();
int ticos_hal_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);
int ticos_hal_mqtt_subscribe(const char *topic, int qos);

static char ticos_client_id[128];
static char ticos_device_id[128];
static char ticos_device_secret[64];
static char ticos_command_request_topic[128];
static char ticos_property_desired_topic[128];
char ticos_property_report_topic[128];
char ticos_telemery_topic[128];

int ticos_cloud_start(const char* product_id, const char* device_id, const char *device_secret)
{
    sprintf(ticos_client_id, "%s@@@%s", device_id, product_id);
    sprintf(ticos_device_id, "%s", device_id);
    sprintf(ticos_device_secret, "%s", device_secret);
    sprintf(ticos_command_request_topic, "devices/%s/commands/request", device_id);
    sprintf(ticos_property_desired_topic, "devices/%s/twin/desired", device_id);
    sprintf(ticos_property_report_topic, "devices/%s/twin/reported", device_id);
    sprintf(ticos_telemery_topic, "devices/%s/telemetry", device_id);

    return ticos_hal_mqtt_start("mqtt://hub.ticos.cn", 1883, ticos_client_id, ticos_device_id, ticos_device_secret);
}

void ticos_cloud_stop()
{
    ticos_hal_mqtt_stop();
}

int ticos_mqtt_subscribe()
{
    int ret = ticos_hal_mqtt_subscribe(ticos_property_desired_topic, 1);
    if (!ret)
        return ticos_hal_mqtt_subscribe(ticos_command_request_topic, 1);
    return ret;
}

void ticos_command_receive(const char *dat, int len);
void ticos_property_receive(const char *dat, int len);
void ticos_msg_recv(const char *topic, const char *dat, int len)
{
    if (!strncmp(topic, ticos_command_request_topic, strlen(ticos_command_request_topic))) {
        ticos_command_receive(dat, len);
    } else if (!strncmp(topic, ticos_property_desired_topic, strlen(ticos_property_desired_topic))) {
        ticos_property_receive(dat, len);
    }
}
