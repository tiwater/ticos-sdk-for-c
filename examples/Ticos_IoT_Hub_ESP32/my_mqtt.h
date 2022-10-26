#ifndef __MY_MQTT_H
#define __MY_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

int my_mqtt_start(const char* fqdn,
                    const char* product_id,
                    const char* device_id);

int my_mqtt_stop(void);

#ifdef __cplusplus
}
#endif

#endif // __MY_MQTT_H
