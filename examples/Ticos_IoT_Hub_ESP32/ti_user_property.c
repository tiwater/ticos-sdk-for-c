#include "ti_thingmodel.h"
#include "ti_core.h"
#include "ti_iot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

extern const ti_iot_telemetry_info_t ti_iot_telemetry_tab[TICOS_IOT_TELEMETRY_MAX];
extern const ti_iot_property_info_t ti_iot_property_tab[TICOS_IOT_PROPERTY_MAX];
extern const ti_iot_command_info_t ti_iot_command_tab[TICOS_IOT_COMMAND_MAX];

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
    for (int i = 0; i < TICOS_IOT_PROPERTY_MAX; i++)
       payload = __ti_iot_property_msg_pack(i, payload);
    return payload;
}

ti_span ti_iot_property_msg_pack_by_id(int index, ti_span payload)
{
    if (index >= TICOS_IOT_PROPERTY_MAX)
        return payload;

    return __ti_iot_property_msg_pack(index, payload);
}

ti_span ti_iot_property_msg_pack_by_name(const char *prop, ti_span payload)
{
    int i = 0;
    for (i = 0; i < TICOS_IOT_PROPERTY_MAX; i ++)
        if (!strcmp(prop, ti_iot_property_tab[i].id))
            break;

    if (i >= TICOS_IOT_PROPERTY_MAX)
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
    for (int i = 0; i < TICOS_IOT_TELEMETRY_MAX; i++)
       payload = __ti_iot_telemetry_msg_pack(i, payload);
    return payload;
}

ti_span ti_iot_telemetry_msg_pack_by_id(int index, ti_span payload)
{
    if (index >= TICOS_IOT_TELEMETRY_MAX)
        return payload;

    return __ti_iot_telemetry_msg_pack(index, payload);
}

ti_span ti_iot_telemetry_msg_pack_by_name(const char *prop, ti_span payload)
{
    int i = 0;
    for (i = 0; i < TICOS_IOT_TELEMETRY_MAX; i ++)
        if (!strcmp(prop, ti_iot_telemetry_tab[i].id))
            break;

    if (i >= TICOS_IOT_TELEMETRY_MAX)
        return payload;

    return __ti_iot_telemetry_msg_pack(i, payload);
}

void ti_iot_command_parse(const char *dat, int len)
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
    for (i = 0; i < TICOS_IOT_COMMAND_MAX; i++)
        if (ti_json_token_is_text_equal(&json_parse.token,
                ti_span_create_from_str((char *)ti_iot_command_tab[i].id)))
            break;

    if (i >= TICOS_IOT_COMMAND_MAX)
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

