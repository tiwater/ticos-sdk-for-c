#include "ti_thingmodel.h"
#include "ti_core.h"
#include "ti_iot.h"
#include <string.h>
#include <stdio.h>

extern const ti_iot_prop_info_t ti_iot_prop_info_get_tab[];
extern const ti_iot_prop_info_t ti_iot_prop_info_set_tab[];
extern const void *ti_iot_get_func_tab[];
extern const void *ti_iot_set_func_tab[];
extern const int read_prop_cnt;
extern const int write_prop_cnt;

typedef int (*_ti_iot_get_int_t)();
typedef int (*_ti_iot_get_bool_t)();
typedef float (*_ti_iot_get_float_t)();
typedef const char* (*_ti_iot_get_string_t)();

typedef void (*_ti_iot_set_int_t)(int);
typedef void (*_ti_iot_set_bool_t)(int);
typedef void (*_ti_iot_set_float_t)(float);
typedef void (*_ti_iot_set_string_t)(const char*);

static ti_span __ti_iot_property_msg_pack(int index, ti_span payload)
{
    char buf[128];

    if (index == 0)
        sprintf(buf, "\"%s\":", ti_iot_prop_info_get_tab[index].id);
    else
        sprintf(buf, ", \"%s\":", ti_iot_prop_info_get_tab[index].id);

    payload = ti_span_copy(payload, ti_span_create_from_str(buf));

    switch (ti_iot_prop_info_get_tab[index].type) {
    case TICOS_IOT_VAL_TYPE_BOOLEAN: {
        _ti_iot_get_bool_t ti_iot_get_bool = (_ti_iot_get_bool_t)ti_iot_get_func_tab[index];
        if (ti_iot_get_bool())
            strcpy(buf, "true");
        else
            strcpy(buf, "false");
        payload = ti_span_copy(payload, ti_span_create_from_str(buf));
        break;
    }
    case TICOS_IOT_VAL_TYPE_INTEGER:
    case TICOS_IOT_VAL_TYPE_ENUM: {
        _ti_iot_get_int_t ti_iot_get_int = (_ti_iot_get_int_t)ti_iot_get_func_tab[index];
        ti_span_i32toa(payload, ti_iot_get_int(), &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_FLOAT: {
        _ti_iot_get_float_t ti_iot_get_float = (_ti_iot_get_float_t)ti_iot_get_func_tab[index];
        ti_span_dtoa(payload, ti_iot_get_float(), 3, &payload);
        break;
    }
    case TICOS_IOT_VAL_TYPE_STRING: {
        _ti_iot_get_string_t ti_iot_get_string = (_ti_iot_get_string_t)ti_iot_get_func_tab[index];
        sprintf(buf, "\"%s\"", ti_iot_get_string());
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
    for (int i = 0; i < read_prop_cnt; i++)
       payload = __ti_iot_property_msg_pack(i, payload);
    return payload;
}

ti_span ti_iot_property_msg_pack_by_id(int index, ti_span payload)
{
    if (index >= read_prop_cnt)
        return payload;

    return __ti_iot_property_msg_pack(index, payload);
}

ti_span ti_iot_property_msg_pack_by_name(const char *prop, ti_span payload)
{
    int i = 0;
    for (i = 0; i < read_prop_cnt; i ++)
        if (!strcmp(prop, ti_iot_prop_info_get_tab[i].id))
            break;

    if (i >= read_prop_cnt)
        return payload;

    return __ti_iot_property_msg_pack(i, payload);
}
