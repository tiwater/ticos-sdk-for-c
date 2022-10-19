#pragma once

#include <_ti_cfg_prefix.h>

void ti_iot_cloud_start();
void ti_iot_cloud_stop();

int ti_iot_property_report(void);
void ti_iot_property_receive(const char *dat, int len);

void ti_iot_command_receive(const char *dat, int len);

#include <_ti_cfg_suffix.h>
