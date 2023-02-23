// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <esp_err.h>
#include <esp_event.h>

#include "ticos_esp_mem.h"
#include "ticos_esp_utils.h"

#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief QCloud Length of configuration information.
 */
#define CLIENT_ID_MAX_SIZE               (80)  /**< MAX size of client ID */
#define PRODUCT_ID_SIZE                  (32)  /**< MAX size of product ID */
#define PRODUCT_SECRET_MAX_SIZE          (32)  /**< MAX size of product secret */
#define DEVICE_NAME_MAX_SIZE             (128)  /**< MAX size of device name */
#define DEVICE_SECRET_SIZE               (128)  /**< MAX size of device secret */
#define DEVICE_CERT_FILE_DEFAULT_SIZE    (92) /**< MAX size of device cert file name */

#define AUTH_TOKEN_MAX_SIZE              (32)  /**< MAX size of auth token */

/**
 * @brief ESP QCloud Event Base.
 */
ESP_EVENT_DECLARE_BASE(QCLOUD_EVENT);

/**
 * @brief ESP QCloud Events.
 */
typedef enum {
    QCLOUD_EVENT_IOTHUB_INIT_DONE = 1,    /**< QCloud core Initialisation Done */
    QCLOUD_EVENT_IOTHUB_BOND_DEVICE,      /**< QCloud bind device */
    QCLOUD_EVENT_IOTHUB_UNBOND_DEVICE,    /**< QCloud unbind device */
    QCLOUD_EVENT_IOTHUB_BIND_EXCEPTION,   /**< QCloud bind exception */
    QCLOUD_EVENT_IOTHUB_RECEIVE_STATUS,   /**< QCloud receive status message */
    QCLOUD_EVENT_LOG_FLASH_FULL,          /**< QCloud log storage full */
} ticos_esp_iothub_event_t;

/**
 * @brief ESP QCloud Auth mode.
 */
typedef enum {
    QCLOUD_AUTH_MODE_INVALID,       /**< Invalid mode */
    QCLOUD_AUTH_MODE_KEY,           /**< Key authentication */
    QCLOUD_AUTH_MODE_CERT,          /**< Certificate authentication */ 
    QCLOUD_AUTH_MODE_DYNREG,        /**< Dynamic authentication */
} ticos_esp_auth_mode_t;

typedef enum {
    QCLOUD_REPORT_EVENT_TYPE_INVALID = 0, /**< Invalid */
    QCLOUD_REPORT_EVENT_TYPE_INFO,        /**< Info */
    QCLOUD_REPORT_EVENT_TYPE_ALERT,       /**< Alert */
    QCLOUD_REPORT_EVENT_TYPE_FAULT,       /**< Fault */
} ticos_esp_event_type_t;

/**
 * @brief ESP QCloud Parameter Value type.
 */
typedef enum {
    QCLOUD_VAL_TYPE_INVALID = 0, /**< Invalid */
    QCLOUD_VAL_TYPE_BOOLEAN,     /**< Boolean */
    QCLOUD_VAL_TYPE_INTEGER,     /**< Integer. Mapped to a 32 bit signed integer */
    QCLOUD_VAL_TYPE_FLOAT,       /**< Floating point number */
    QCLOUD_VAL_TYPE_STRING,      /**< NULL terminated string */
    QCLOUD_VAL_TYPE_STRUCT,      /**< Transmission as a string */
    QCLOUD_VAL_TYPE_ENUM,        /**< Transmission as a integer */
    QCLOUD_VAL_TYPE_TIME,        /**< Transmission as a integer */
} ticos_esp_param_val_type_t;

/**
 * @brief ESP QCloud Value.
 */
typedef struct {
    ticos_esp_param_val_type_t type;
    float f;     /**< Float */
    union {
        bool b;  /**< Boolean */
        int i;   /**< Integer */
        char *s; /**< NULL terminated string */
    };
} ticos_esp_param_val_t;

/**
 * @brief Types of methods for reporting to the cloud
 * 
 */
typedef enum {
    QCLOUD_METHOD_TYPE_INVALID = 0, /**< Invalid */
    QCLOUD_METHOD_TYPE_EVENT,
    QCLOUD_METHOD_TYPE_ACTION_REPLY,  
    QCLOUD_METHOD_TYPE_APP_BIND_TOKEN,
    QCLOUD_METHOD_TYPE_REPORT,
    QCLOUD_METHOD_TYPE_REPORT_INFO, 
    QCLOUD_METHOD_TYPE_MAX_INVALID,
} ticos_esp_method_type_t;

typedef struct ticos_esp_param {
    const char *id;
    ticos_esp_param_val_t value;
    SLIST_ENTRY(ticos_esp_param) next;    //!< next command in the list
} ticos_esp_param_t;

typedef struct ticos_esp_method_extra{
    double timestamp;        /**< Exist only in event_post and report*/
    char *type;              /**< Exist only in event_post*/
    char *version;           /**< Exist only in event_post*/
    char *id;                /**< Exist only in event_post*/
    char *token;             /**< Exist only in action_reply and control reply*/
    uint32_t code;           /**< Exist only in action_reply and control reply*/
}ticos_esp_method_extra_val_t;

typedef struct ticos_esp_skin_res{
    //char acid[40];
    //char userid[40];
    int skinscore;
    //int monthtimes;
    //char imageurl[200];
    //char reportUrl[200];
    int status;         // 1. 检测中.  2 检测完成            3 检测失败.
    //char createdtime[40];
    char summary[200];
    //char productid[20];
    //char devicename[20];
}ticos_esp_skin_res_t;
typedef struct ticos_esp_method {
    ticos_esp_method_type_t method_type;
    ticos_esp_method_extra_val_t *extra_val;
    SLIST_HEAD(method_param_list_, ticos_esp_param) method_param_list;
} ticos_esp_method_t;

/**
 * @brief Interface method.
 * 
 */
typedef esp_err_t (*ticos_esp_action_cb_t)(ticos_esp_method_t *action_handle, char *params);

typedef struct ticos_esp_action {
    const char *id;
    ticos_esp_action_cb_t action_cb;
    SLIST_ENTRY(ticos_esp_action) next;    //!< next command in the list
} ticos_esp_action_t;

/**
 * @brief Interface method get_param.
 * 
 */
typedef esp_err_t (*ticos_esp_get_param_t)(const char *id, ticos_esp_param_val_t *val);

/**
 * @brief Interface method set_param.
 * 
 */
typedef esp_err_t (*ticos_esp_set_param_t)(const char *id, const ticos_esp_param_val_t *val);

/*
    @brief 获取测肤结果.
*/
//ticos_esp_skin_res_t *ticos_esp_get_skin_res(void);
/**
 * @brief Add device property callback function, set and get.
 * 
 * @param[in] get_param_cb Get param interface.
 * @param[in] set_param_cb Set param interface.
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_device_add_property_cb(const ticos_esp_get_param_t get_param_cb,
                                   const ticos_esp_set_param_t set_param_cb);


/**
 * @brief Add device action callback function.
 * 
 * @param[in] action_id Parameter id.
 * @param[in] action_cb Callback
 * @return esp_err_t 
 */
esp_err_t ticos_esp_device_add_action_cb(const char *action_id, const ticos_esp_action_cb_t action_cb);

/**
 * @brief Create device.
 * 
 * @note Need product id, device name, device key.
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_create_device(char* _product_id, char* _device_id, char* _device_secret);

/**
 * @brief Add firmware version information.
 * 
 * @param[in] version Current firmware version.
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_device_add_fw_version(const char *version);

/**
 * @brief Get firmware version information.
 * 
 * @return Pointer to firmware version.
 */
const char *ticos_esp_get_version(void);

/**
 * @brief Get device name.
 * 
 * @return Pointer to device name.
 */
const char *ticos_esp_get_device_name(void);

/**
 * @brief Get product id.
 * 
 * @return Pointer to product id.
 */
const char *ticos_esp_get_product_id(void);

/**
 * @brief Get authentication mode.
 * 
 * @return Current authentication model.
 */
ticos_esp_auth_mode_t ticos_esp_get_auth_mode(void);

/**
 * @brief Get device secret.
 * 
 * @return Pointer to device secret.
 */
const char *ticos_esp_get_device_secret(void);

/**
 * @brief Get Certification.
 * 
 * @return Pointer to certification.
 */
const char *ticos_esp_get_cert_crt(void);

/**
 * @brief Get private key.
 * 
 * @return Pointer to private key.
 */
const char *ticos_esp_get_private_key(void);
/**
 * @brief Get mac+pid.
 * 
 * @return Pointer to device info for ble adv.
 */
int ble_get_device_info(char *device_info);
/**
 * @brief Add properties to your device
 * 
 * @note You need to register these properties on Qcloud, Ensure property identifier is correct.
 * 
 * @param[in] id property identifier.
 * @param[in] type property type.
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_device_add_property(const char *id, ticos_esp_param_val_type_t type);

/**
 * @brief Set local properties.
 * 
 * @note When a control message is received, the function will be called.
 * This is an internal function, You can not modify it.
 * You need to pass your function through ticos_esp_device_add_property_cb.
 * 
 * @param[in] request_params 
 * @param[in] reply_data 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_handle_set_param(const cJSON *request_params, cJSON *reply_data);

/**
 * @brief Get local properties.
 * 
 * @note When a control message is received, the function will be called.
 * This is an internal function, You can not modify it.
 * You need to pass your function through ticos_esp_device_add_property_cb.
 * 
 * @param[in] request_data 
 * @param[in] reply_data 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_handle_get_param(const cJSON *request_data, cJSON *reply_data);

/**
 * @brief Initialize Qcloud and establish MQTT service.
 * 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_init(void);

/**
 * @brief Run Qcloud service and register related parameters.
 * 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_start(void);

/**
 * @brief Stop Qcloud service and release related resources.
 * 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_stop(void);

/**
 * @brief Use token to bind with Qcloud.
 * 
 * @note If the blocking option is selected, the function will wait for the binding result 
 * returned by the cloud platform. You can also listen to the <QCLOUD_EVENT> event to get the result.
 * 
 * @param[in] token Token comes from WeChat applet.
 * @param[in] block Blocking option
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_bind(const char *token, bool block);

/**
 * @brief Get the latest status of the QCloud
 * 
 * @param[in] type Message type
 * @param[in] auto_update When the status is obtained, whether to automatically call the set function.
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_get_status(ticos_esp_method_type_t type, bool auto_update);

/**
 * @brief Report device information.
 * 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_report_device_info(void);

/**
 * @brief Report all property.
 * 
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_report_all_property(void);

/**
 * @brief Get Qcloud service status.
 * 
 * @return true Connect
 * @return false Disconnect
 */
bool ticos_esp_iothub_is_connected(void);

/** 
 * @brief Enable OTA
 *
 * @note Calling this API enables OTA as per the ESP QCloud specification.
 * Please check the various ESP QCloud configuration options to
 * use the different variants of OTA. Refer the documentation for
 * additional details.
 *
 * @return
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_ota_enable(void);

/**
 * @brief Add int type data to the handle method.
 * 
 * @param[in] method Method handle.
 * @param[in] id Parameter id.
 * @param[in] value Parameter value.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_param_add_int(ticos_esp_method_t *method, char *id, int value);

/**
 * @brief Add float type data to the handle method.
 * 
 * @param[in] method Method handle.
 * @param[in] id Parameter id.
 * @param[in] value Parameter value.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_param_add_float(ticos_esp_method_t *method, char *id, float value);

/**
 * @brief Add char type data to the handle method.
 * 
 * @param[in] method Method handle.
 * @param[in] id Parameter id.
 * @param[in] value Parameter value.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_param_add_string(ticos_esp_method_t *method, char *id, char *value);

/**
 * @brief Add bool type data to the handle method.
 * 
 * @param[in] method Method handle.
 * @param[in] id Parameter id.
 * @param[in] value Parameter value.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_param_add_bool(ticos_esp_method_t *method, char *id, bool value);

/**
 * @brief Create a report handle.
 * 
 * @return Pointer to the report handle.
 */
ticos_esp_method_t *ticos_esp_iothub_create_report(void);

/**
 * @brief Create a event handle.
 * 
 * @param[in] eventId Parameter id.
 * @param[in] type  Event type
 * @return Pointer to the event handle.
 */
ticos_esp_method_t *ticos_esp_iothub_create_event(const char *eventId, ticos_esp_event_type_t type);

/**
 * @brief Create a action handle.
 * 
 * @return Pointer to the report handle.
 */
ticos_esp_method_t *ticos_esp_iothub_create_action(void);

/**
 * @brief Destroy the handle by report.
 * 
 * @param[in] report Method handle.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_destroy_report(ticos_esp_method_t *report);

/**
 * @brief Destroy the handle by event.
 * 
 * @param[in] event Method handle.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_destroy_event(ticos_esp_method_t *event);

/**
 * @brief Destroy the handle by action.
 * 
 * @param[in] event Method handle.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_destroy_action(ticos_esp_method_t *action);

/**
 * @brief Post handle data to QCloud.
 * 
 * @param[in] method Method handle.
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_iothub_post_method(ticos_esp_method_t *method);

/**
 * @brief The device executes actions from the cloud.
 * 
 * @param[in] action_handle Method handle.
 * @param[in] action_id Parameter id.
 * @param[in] params Downstream parameters
 * @return  
 *     - ESP_OK: succeed
 *     - others: fail
 */
esp_err_t ticos_esp_operate_action(ticos_esp_method_t *action_handle, const char *action_id, char *params);

#ifdef __cplusplus
}
#endif
