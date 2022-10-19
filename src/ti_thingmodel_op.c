#include "ti_thingmode_type.h"
#include "ti_core.h"
#include "ti_iot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#define TRUE    "true"
#define FALSE   "false"

typedef int (*_ti_iot_upload_int_t)();
typedef int (*_ti_iot_upload_bool_t)();
typedef float (*_ti_iot_upload_float_t)();
typedef const char* (*_ti_iot_upload_string_t)();

typedef void (*_ti_iot_download_int_t)(int);
typedef void (*_ti_iot_download_bool_t)(int);
typedef void (*_ti_iot_download_float_t)(float);
typedef void (*_ti_iot_download_string_t)(const char*);

extern const ti_iot_telemetry_info_t ti_iot_telemetry_tab[];
extern const ti_iot_property_info_t ti_iot_property_tab[];
extern const ti_iot_command_info_t ti_iot_command_tab[];
extern const int ti_iot_telemetry_cnt;
extern const int ti_iot_property_cnt;
extern const int ti_iot_command_cnt;

int ti_iot_mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);
const char *ti_iot_get_device_id(void);

static ti_span __ti_iot_property_msg_pack(int index, ti_span payload)
{
    char buf[128];

    if (index == 0)
        sprintf(buf, "\"%s\":", ti_iot_property_tab[index].id);
    else
        sprintf(buf, ", \"%s\":", ti_iot_property_tab[index].id);

    payload = ti_span_copy(payload, ti_span_create_from_str(buf));
    void *upload_func = ti_iot_property_tab[index].upload_func;

    switch (ti_iot_property_tab[index].type) {
    case TICOS_IOT_VAL_TYPE_BOOLEAN: {
        if (((_ti_iot_upload_bool_t)upload_func)())
            strcpy(buf, TRUE);
        else
            strcpy(buf, FALSE);
        payload = ti_span_copy(payload, ti_span_create_from_str(buf));
        break;
    }
    case TICOS_IOT_VAL_TYPE_INTEGER: {
        ti_span_i32toa(payload, ((_ti_iot_upload_int_t)upload_func)(), &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_FLOAT: {
        ti_span_dtoa(payload, ((_ti_iot_upload_float_t)upload_func)(), 3, &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_STRING: {
        sprintf(buf, "\"%s\"", ((_ti_iot_upload_string_t)upload_func)());
        payload = ti_span_copy(payload, ti_span_create_from_str(buf));
        break;
    }
    default:
        break;
    }

    return payload;
}

ti_span ti_iot_property_msgs_pack(ti_span payload)
{
    for (int i = 0; i < ti_iot_property_cnt; i++)
       payload = __ti_iot_property_msg_pack(i, payload);
    return payload;
}

ti_span ti_iot_property_msg_pack_by_id(int index, ti_span payload)
{
    if (index >= ti_iot_property_cnt)
        return payload;

    return __ti_iot_property_msg_pack(index, payload);
}

ti_span ti_iot_property_msg_pack_by_name(const char *prop, ti_span payload)
{
    int i = 0;
    for (i = 0; i < ti_iot_property_cnt; i ++)
        if (!strcmp(prop, ti_iot_property_tab[i].id))
            break;

    if (i >= ti_iot_property_cnt)
        return payload;

    return __ti_iot_property_msg_pack(i, payload);
}


static ti_span __ti_iot_telemetry_msg_pack(int index, ti_span payload)
{
    char buf[128];

    if (index == 0)
        sprintf(buf, "\"%s\":", ti_iot_telemetry_tab[index].id);
    else
        sprintf(buf, ", \"%s\":", ti_iot_telemetry_tab[index].id);

    payload = ti_span_copy(payload, ti_span_create_from_str(buf));
    void *func = ti_iot_telemetry_tab[index].func;

    switch (ti_iot_telemetry_tab[index].type) {
    case TICOS_IOT_VAL_TYPE_BOOLEAN: {
        if (((_ti_iot_upload_bool_t)func)())
            strcpy(buf, TRUE);
        else
            strcpy(buf, FALSE);
        payload = ti_span_copy(payload, ti_span_create_from_str(buf));
        break;
    }
    case TICOS_IOT_VAL_TYPE_INTEGER: {
        ti_span_i32toa(payload, ((_ti_iot_upload_int_t)func)(), &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_FLOAT: {
        ti_span_dtoa(payload, ((_ti_iot_upload_float_t)func)(), 3, &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_STRING: {
        sprintf(buf, "\"%s\"", ((_ti_iot_upload_string_t)func)());
        payload = ti_span_copy(payload, ti_span_create_from_str(buf));
        break;
    }
    default:
        break;
    }

    return payload;
}

ti_span ti_iot_telemetry_msgs_pack(ti_span payload)
{
    for (int i = 0; i < ti_iot_telemetry_cnt; i++)
       payload = __ti_iot_telemetry_msg_pack(i, payload);
    return payload;
}

ti_span ti_iot_telemetry_msg_pack_by_id(int index, ti_span payload)
{
    if (index >= ti_iot_telemetry_cnt)
        return payload;

    return __ti_iot_telemetry_msg_pack(index, payload);
}

ti_span ti_iot_telemetry_msg_pack_by_name(const char *prop, ti_span payload)
{
    int i = 0;
    for (i = 0; i < ti_iot_telemetry_cnt; i ++)
        if (!strcmp(prop, ti_iot_telemetry_tab[i].id))
            break;

    if (i >= ti_iot_telemetry_cnt)
        return payload;

    return __ti_iot_telemetry_msg_pack(i, payload);
}

void ti_iot_command_receive(const char *dat, int len)
{
    ti_json_reader json_parse;
    ti_result ret = ti_json_reader_init(&json_parse, ti_span_create((uint8_t *)dat, len), NULL);
    if (ret != TI_OK)
        return;
    do {
        ret = ti_json_reader_next_token(&json_parse);
        if (ret != TI_OK)
            return;
    } while (json_parse.token.kind != TI_JSON_TOKEN_PROPERTY_NAME);

    int i;
    for (i = 0; i < ti_iot_command_cnt; i++)
        if (ti_json_token_is_text_equal(&json_parse.token,
                ti_span_create_from_str((char *)ti_iot_command_tab[i].id)))
            break;

    if (i >= ti_iot_command_cnt)
        return;

    ret = ti_json_reader_next_token(&json_parse);
    if (ret != TI_OK)
        return;

    char str[128];
    ret = ti_json_token_get_string(&json_parse.token, str, sizeof(str), &len);
    if (ret != TI_OK)
        return;

    str[len] = 0;
    void *command_func = ti_iot_command_tab[i].func;

    switch (ti_iot_command_tab[i].type) {
    case TICOS_IOT_VAL_TYPE_BOOLEAN:
        if (!strcmp(str, TRUE))
            ((_ti_iot_download_bool_t)command_func)(1);
        else
            ((_ti_iot_download_bool_t)command_func)(0);
        break;
    case TICOS_IOT_VAL_TYPE_INTEGER:
        ((_ti_iot_download_int_t)command_func)(atoi(str));
        break;
    case TICOS_IOT_VAL_TYPE_FLOAT:
        ((_ti_iot_download_float_t)command_func)(atof(str));
        break;
    case TICOS_IOT_VAL_TYPE_STRING:
        ((_ti_iot_download_string_t)command_func)(str);
        break;
    default:
        break;
    }
}

static cJSON *__ti_iot_property_pack(int index)
{
    cJSON *property = cJSON_CreateObject();
    cJSON_AddStringToObject(property, "op", "add");

    char path[128];
    sprintf(path, "/%s", ti_iot_property_tab[index].id);
    cJSON_AddStringToObject(property, "path", path);

    void *func = ti_iot_property_tab[index].upload_func;

    switch (ti_iot_property_tab[index].type) {
    case TICOS_IOT_VAL_TYPE_BOOLEAN:
        cJSON_AddBoolToObject(property, "value", ((_ti_iot_upload_bool_t)func)());
        break;
    case TICOS_IOT_VAL_TYPE_INTEGER:
        cJSON_AddNumberToObject(property, "value", ((_ti_iot_upload_int_t)func)());
        break;
    case TICOS_IOT_VAL_TYPE_FLOAT:
        cJSON_AddNumberToObject(property, "value", ((_ti_iot_upload_float_t)func)());
        break;
    case TICOS_IOT_VAL_TYPE_STRING:
        cJSON_AddStringToObject(property, "value", ((_ti_iot_upload_string_t)func)());
        break;
    default:
        cJSON_AddNullToObject(property, "value");
        break;
    }

    return property;
}

static cJSON *ti_iot_property_pack(void)
{
    cJSON *propertys = cJSON_CreateArray();
    for (int i = 0; i < ti_iot_property_cnt; i++) {
        cJSON *property = __ti_iot_property_pack(i);
        cJSON_AddItemReferenceToArray(propertys, property);
    }
    return propertys;
}

static cJSON *ti_iot_property_pack_by_id(int index)
{
    if (index >= ti_iot_property_cnt)
        return NULL;

    cJSON *propertys = cJSON_CreateArray();
    cJSON *property = __ti_iot_property_pack(index);
    cJSON_AddItemReferenceToArray(propertys, property);
    return propertys;
}

static cJSON *ti_iot_property_pack_by_name(const char *prop)
{
    for (int i = 0; i < ti_iot_property_cnt; i ++)
        if (!strcmp(prop, ti_iot_property_tab[i].id))
            return __ti_iot_property_pack(i);
    return NULL;
}

static int ti_iot_property_free(cJSON *property)
{
    if (!cJSON_IsArray(property))
        return -1;

    cJSON *_pro = cJSON_DetachItemFromArray(property, 0);
    while (_pro) {
        cJSON_Delete(_pro);
        _pro = cJSON_DetachItemFromArray(property, 0);
    }
    cJSON_Delete(property);
    return 0;
}

void ti_iot_property_receive(const char *dat, int len)
{
    cJSON *propretys = cJSON_Parse(dat);
    if (!propretys)
        return;

    char *log_info = cJSON_Print(propretys);
    printf("property receive: \n%s\n", log_info);
    cJSON_free(log_info);

    cJSON *metadata = cJSON_GetObjectItem(propretys, "$metadata");
    if (metadata && cJSON_IsObject(metadata)) {
        int size = cJSON_GetArraySize(metadata);
        for (int i = 0; i < size; i++) {
            cJSON *property = cJSON_GetArrayItem(metadata, i);
            if ((!property) || (!cJSON_IsObject(property)))
                continue;
            cJSON *value = cJSON_GetObjectItem(property, "desiredValue");
            if (!value)
                continue;
            for (int j = 0; j < ti_iot_property_cnt; j++) {
                if (!strcmp(ti_iot_property_tab[j].id, property->string)) {
                    void *download_func = ti_iot_property_tab[j].download_func;
                    switch (ti_iot_property_tab[j].type) {
                    case TICOS_IOT_VAL_TYPE_BOOLEAN:
                        if (cJSON_IsBool(value))
                            ((_ti_iot_download_bool_t)download_func)(cJSON_IsTrue(value));
                    case TICOS_IOT_VAL_TYPE_INTEGER:
                        if (cJSON_IsNumber(value))
                            ((_ti_iot_download_int_t)download_func)(cJSON_GetNumberValue(value));
                        break;
                    case TICOS_IOT_VAL_TYPE_FLOAT:
                        if (cJSON_IsNumber(value))
                            ((_ti_iot_download_float_t)download_func)(cJSON_GetNumberValue(value));
                        break;
                    case TICOS_IOT_VAL_TYPE_STRING:
                        if (cJSON_IsString(value))
                            ((_ti_iot_download_string_t)download_func)(cJSON_GetStringValue(value));
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }
        }
    }
    cJSON_Delete(propretys);
}

int ti_iot_property_report(void)
{
    cJSON *propretys = ti_iot_property_pack();
    if (!propretys)
        return -1;
    char *propertys_str = cJSON_Print(propretys);
    printf("property report: \n%s\n", propertys_str);

    char report_topic[128];
    sprintf(report_topic, "devices/%s/twin/patch/reported", ti_iot_get_device_id());
    int ret = ti_iot_mqtt_client_publish(report_topic, propertys_str, strlen(propertys_str), 1, 0);
    cJSON_free(propertys_str);
    ti_iot_property_free(propretys);
    return ret;
}

