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

#include <string.h>

#include <esp_log.h>

#include "ticos_esp_iothub.h"
#include "ticos_esp_utils.h"
#include "esp_bt_device.h"
#include "ticos_api.h"
#ifdef CONFIG_QCLOUD_MASS_MANUFACTURE
#include "nvs.h"
#include "nvs_flash.h"
#endif

/* Handle to maintain internal information (will move to an internal file) */
typedef struct {
    char *product_id;
    char *version;
    ticos_esp_auth_mode_t auth_mode;
    char *device_name;
    union {
        char *device_secret;
        struct  {
            char *cert_crt;
            char *private_key;
        };
    };
} ticos_esp_profile_t;

static ticos_esp_profile_t *g_device_profile = NULL;
static ticos_esp_set_param_t g_ticos_esp_set_param = NULL;
static ticos_esp_get_param_t g_ticos_esp_get_param = NULL;

static SLIST_HEAD(param_list_, ticos_esp_param) g_property_list;
static SLIST_HEAD(action_list_, ticos_esp_action) g_action_list;

#ifdef CONFIG_AUTH_MODE_CERT
extern const uint8_t dev_cert_crt_start[] asm("_binary_dev_cert_crt_start");
extern const uint8_t dev_cert_crt_end[] asm("_binary_dev_cert_crt_end");
extern const uint8_t dev_private_key_start[] asm("_binary_dev_private_key_start");
extern const uint8_t dev_private_key_end[] asm("_binary_dev_private_key_end");
#endif

static const char *TAG = "ticos_esp_device";

esp_err_t ticos_esp_device_add_fw_version(const char *version)
{
    ticos_esp_PARAM_CHECK(version);

    g_device_profile->version = strdup(version);

    return ESP_OK;
}

esp_err_t ticos_esp_device_secret(const char *device_secret)
{
    ticos_esp_PARAM_CHECK(device_secret && strlen(device_secret) == DEVICE_SECRET_SIZE);

    return ESP_OK;
}

esp_err_t ticos_esp_device_cert(const char *cert_crt, const char *private_key)
{
    ticos_esp_PARAM_CHECK(cert_crt);
    ticos_esp_PARAM_CHECK(private_key);

    g_device_profile->cert_crt    = strdup(cert_crt);
    g_device_profile->private_key = strdup(private_key);
    g_device_profile->auth_mode   = QCLOUD_AUTH_MODE_CERT;

    return ESP_OK;
}

esp_err_t ticos_esp_create_device(char* _product_id, char* _device_id, char* _device_secret)
{
    g_device_profile = ticos_esp_CALLOC(1, sizeof(ticos_esp_profile_t));

#ifdef CONFIG_QCLOUD_MASS_MANUFACTURE
    g_device_profile->auth_mode = QCLOUD_AUTH_MODE_KEY;
    /**
     * @brief Read device configuration information through flash
     *        1. Configure device information via single_mfg_config.csv
     *        2. Generate single_mfg_config.bin, use the following command:
     *          python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate single_mfg_config.csv single_mfg_config.bin 0x4000
     *        3. Burn single_mfg_config.bin to flash
     *          python $IDF_PATH/components/esptool_py/esptool/esptool.py write_flash 0x15000 single_mfg_config.bin
     * @note Currently does not support the use of certificates for mass manufacture
     */
    esp_err_t err = ESP_FAIL;
    nvs_handle handle = 0;
    size_t required_size = 0;
    ESP_ERROR_CHECK(nvs_flash_init_partition(CONFIG_QCLOUD_FACTRY_PARTITION_NAME));
    err = nvs_open_from_partition(CONFIG_QCLOUD_FACTRY_PARTITION_NAME,
                                  CONFIG_QCLOUD_FACTRY_PARTITION_NAMESPACE, NVS_READONLY, &handle);
    ticos_esp_ERROR_GOTO(err != ESP_OK, ERR_EXIT, "<%s> Factory partition information is not burned", esp_err_to_name(err));

    required_size = PRODUCT_ID_SIZE + 1;
    g_device_profile->product_id = ticos_esp_CALLOC(1, PRODUCT_ID_SIZE + 1);
    ESP_ERROR_CHECK(nvs_get_str(handle, "product_id", g_device_profile->product_id, &required_size));

    required_size = DEVICE_NAME_MAX_SIZE + 1;
    g_device_profile->device_name = ticos_esp_CALLOC(1, DEVICE_NAME_MAX_SIZE + 1);
    ESP_ERROR_CHECK(nvs_get_str(handle, "device_name", g_device_profile->device_name, &required_size));

    required_size = DEVICE_SECRET_SIZE + 1;
    g_device_profile->device_secret = ticos_esp_CALLOC(1, DEVICE_SECRET_SIZE + 1);
    ESP_ERROR_CHECK(nvs_get_str(handle, "device_secret", g_device_profile->device_secret, &required_size));
#else

    g_device_profile->product_id    = _product_id;
    g_device_profile->device_name   = _device_id;

#ifdef CONFIG_AUTH_MODE_KEY
    g_device_profile->auth_mode = QCLOUD_AUTH_MODE_KEY;
    /**
     * @brief Read device configuration information through sdkconfig.h
     *        1. Configure device information via `idf.py menuconfig`, Menu path: (Top) -> Example Configuration
     *        2. Select key authentication
     *        3. Enter device secret key
     */
    g_device_profile->device_secret = _device_secret;
    ESP_LOGI(TAG, "product_id: %s; device_name: %s; device_secret: %s",
            g_device_profile->product_id, 
            g_device_profile->device_name, 
            g_device_profile->device_secret);

    if (strlen(g_device_profile->device_secret) > DEVICE_SECRET_SIZE
            || strlen(g_device_profile->product_id) > PRODUCT_ID_SIZE) {
        ESP_LOGE(TAG, "Please check if the authentication information of the device is configured");
        ESP_LOGE(TAG, "Obtain authentication configuration information from login qcloud iothut: ");
        //ESP_LOGE(TAG, "https://console.cloud.tencent.com/iotexplorer");
        ESP_LOGE(TAG, "product_id: %s", g_device_profile->product_id);
        ESP_LOGE(TAG, "device_name: %s", g_device_profile->device_name);
        ESP_LOGE(TAG, "device_secret: %s", g_device_profile->device_secret);
        goto ERR_EXIT;
    }
#endif

#ifdef CONFIG_AUTH_MODE_CERT
    g_device_profile->auth_mode = QCLOUD_AUTH_MODE_CERT;
    /**
     * @brief Read device configuration information through sdkconfig.h
     *        1. Configure device information via `idf.py menuconfig`, Menu path: (Top) -> Example Configuration
     *        2. Choose certificate authentication
     *        3. Replace the certificate file in the config directory
     */

    g_device_profile->cert_crt = (char*)dev_cert_crt_start;
    g_device_profile->private_key = (char*)dev_private_key_start;

    if (strlen(g_device_profile->product_id) != PRODUCT_ID_SIZE
        || strlen(g_device_profile->cert_crt) == DEVICE_CERT_FILE_DEFAULT_SIZE) {
        ESP_LOGE(TAG, "Please check if the authentication information of the device is configured");
        ESP_LOGE(TAG, "Obtain authentication configuration information from login qcloud iothut: ");
        ESP_LOGE(TAG, "https://console.cloud.tencent.com/iotexplorer");
        ESP_LOGE(TAG, "product_id: %s", g_device_profile->product_id);
        ESP_LOGE(TAG, "device_name: %s", g_device_profile->device_name);
        ESP_LOGE(TAG, "cert_crt: \r\n%s", g_device_profile->cert_crt);
        ESP_LOGE(TAG, "private_key: \r\n%s", g_device_profile->private_key);
        goto ERR_EXIT;
    }
#endif

#endif

    return ESP_OK;

ERR_EXIT:
    ticos_esp_FREE(g_device_profile);
    vTaskDelay(pdMS_TO_TICKS(3000));
    return ESP_FAIL;
}

const char *ticos_esp_get_device_name()
{
    return g_device_profile->device_name;
}

const char *ticos_esp_get_version()
{
    return g_device_profile->version;
}

const char *ticos_esp_get_product_id()
{
    return g_device_profile->product_id;
}

ticos_esp_auth_mode_t ticos_esp_get_auth_mode()
{
    return g_device_profile->auth_mode;
}

const char *ticos_esp_get_device_secret()
{
    return g_device_profile->device_secret;
}

const char *ticos_esp_get_cert_crt()
{
    return g_device_profile->cert_crt;
}

const char *ticos_esp_get_private_key()
{
    return g_device_profile->private_key;
}

int ble_get_device_info(char *device_info){
    memcpy(device_info, esp_bt_dev_get_address(), 6);
    memcpy(device_info + 6, g_device_profile->product_id, 10);
    return 0;
}

esp_err_t ticos_esp_device_add_property(const char *id, ticos_esp_param_val_type_t type)
{
    ticos_esp_param_t *item = ticos_esp_CALLOC(1, sizeof(ticos_esp_param_t));

    item->id = strdup(id);
    item->value.type = type;

    ticos_esp_param_t *last = SLIST_FIRST(&g_property_list);

    if (last == NULL) {
        SLIST_INSERT_HEAD(&g_property_list, item, next);
    } else {
        SLIST_INSERT_AFTER(last, item, next);
    }

    return ESP_OK;
}

esp_err_t ticos_esp_device_add_action_cb(const char *action_id, const ticos_esp_action_cb_t action_cb)
{
    ticos_esp_action_t *item = ticos_esp_CALLOC(1, sizeof(ticos_esp_action_t));

    item->id = strdup(action_id);
    item->action_cb = action_cb;

    ticos_esp_action_t *last = SLIST_FIRST(&g_action_list);

    if (last == NULL) {
        SLIST_INSERT_HEAD(&g_action_list, item, next);
    } else {
        SLIST_INSERT_AFTER(last, item, next);
    }

    return ESP_OK;
}

esp_err_t ticos_esp_device_add_property_cb(const ticos_esp_get_param_t get_param_cb,
                                   const ticos_esp_set_param_t set_param_cb)
{
    g_ticos_esp_get_param = get_param_cb;
    g_ticos_esp_set_param = set_param_cb;

    return ESP_OK;
}

esp_err_t ticos_esp_handle_set_param(const cJSON *request_params, cJSON *reply_data)
{
    esp_err_t err = ESP_FAIL;

    for (cJSON *item = request_params->child; item; item = item->next) {
        ticos_esp_param_val_t value = {0};

        switch (item->type) {
            case cJSON_False:
                value.b = false;
                break;

            case cJSON_True:
                value.b = true;
                break;

            case cJSON_Number:
                value.i = item->valueint;
                value.f = item->valuedouble;
                break;

            case cJSON_String:
                value.s = item->valuestring;
                break;

            default:
                break;
        }

        err = g_ticos_esp_set_param(item->string, &value);
        ticos_esp_ERROR_BREAK(err != ESP_OK, "<%s> ticos_esp_set_param, id: %s",
                               esp_err_to_name(err), item->string);
    }

    return err;
}

esp_err_t ticos_esp_handle_get_param(const cJSON *request_data, cJSON *reply_data)
{
    esp_err_t err = ESP_FAIL;

    ticos_esp_param_t *param;
    SLIST_FOREACH(param, &g_property_list, next) {
        /* Check if command starts with buf */
        err = g_ticos_esp_get_param(param->id, &param->value);
        ticos_esp_ERROR_BREAK(err != ESP_OK, "ticos_esp_get_param, id: %s", param->id);

        if (param->value.type == QCLOUD_VAL_TYPE_INTEGER) {
            cJSON_AddNumberToObject(reply_data, param->id, param->value.i);
        } else if (param->value.type == QCLOUD_VAL_TYPE_BOOLEAN) {
            cJSON_AddNumberToObject(reply_data, param->id, param->value.b);
        } else if (param->value.type == QCLOUD_VAL_TYPE_STRING) {
            cJSON_AddStringToObject(reply_data, param->id, param->value.s);
        } else if (param->value.type == QCLOUD_VAL_TYPE_FLOAT) {
            cJSON_AddNumberToObject(reply_data, param->id, param->value.f);
        }
    }

    return err;
}

esp_err_t ticos_esp_operate_action(ticos_esp_method_t *action_handle, const char *action_id, char *params)
{
    esp_err_t err = ESP_ERR_NOT_FOUND;

    ticos_esp_action_t *action;
    SLIST_FOREACH(action, &g_action_list, next) {
        if(!strcmp(action->id, action_id)){
            return action->action_cb(action_handle, params);
        }
    }
    ESP_LOGE(TAG, "The callback function of <%s> was not found, Please check <ticos_esp_device_add_action_cb>", action_id);
    return err;
}
