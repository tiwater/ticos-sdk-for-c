#ifndef __TICOS_IMPORT_H
#define __TICOS_IMPORT_H
int ticos_hal_mqtt_start(const char *url, int port, const char *client_id, const char *user_name, const char *passwd);
void ticos_hal_mqtt_stop();
int ticos_hal_mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);
int ticos_hal_mqtt_subscribe(const char *topic, int qos);
int ticos_hal_mqtt_connected(void);

void ticos_data_response(const char *topic, int topic_len, const char *data, int data_len);
char *esp_version_get();
int ticos_system_user_bind(void);
void ticos_system_user_response_receive(const char *dat, int len);
#endif
