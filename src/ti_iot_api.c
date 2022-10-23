#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include <ti_core.h>
#include <ti_iot.h>
#include <ti_iot_hal.h>
#include <ti_iot_api.h>


static char mqtt_client_id[128];
static char mqtt_username[128];
static ti_iot_hub_client client;
 /**
 * @brief 启动ti iot cloud服务
 * @note  该函数先初始化ti_iot_hub_client，然后调用平台相关的mqtt client连接到云端, 用户需要实现hal_mqtt_client_init函数
 */
void ti_iot_cloud_start()
{
  ti_iot_hub_client_options options = ti_iot_hub_client_options_default();
  //TODO: 设置真正的 agent
  options.user_agent = TI_SPAN_FROM_STR("c%2F" TI_SDK_VERSION_STRING "(ard;esp32)");

  if (ti_result_failed(ti_iot_hub_client_init(
          &client,
          ti_span_create_from_str(ti_iot_get_mqtt_fqdn()),
          ti_span_create_from_str(ti_iot_get_device_id()),
          &options)))
  {
    printf("Failed initializing Ticos IoT Hub client\n");
    return;
  }

  ti_span span_client_id = TI_SPAN_FROM_BUFFER(mqtt_client_id);
  span_client_id = ti_span_copy(span_client_id, ti_span_create_from_str(ti_iot_get_device_id()));
  span_client_id = ti_span_copy(span_client_id, TI_SPAN_FROM_STR("@@@"));
  span_client_id = ti_span_copy(span_client_id, ti_span_create_from_str(ti_iot_get_product_id()));

  ti_span span_username = TI_SPAN_FROM_BUFFER(mqtt_username);
  span_username = ti_span_copy(span_username, ti_span_create_from_str(ti_iot_get_device_id()));

  hal_mqtt_client_init(mqtt_client_id, mqtt_username);
}