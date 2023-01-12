# Zephyr Specific Port

## Overview

This directory contains an implementation of the dependency functions needed to
integrate the Ticos SDK into the Zephyr RTOS.

The instructions below assume you have an environment already setup for building
and flashing a Zephyr application. If you do not, see the official
[getting started guide](https://docs.zephyrproject.org/2.0.0/getting_started/index.html#build-hello-world).

## Directories

The subdirectories within the folder are titled to align with the Zephyr release
the porting files were tested against. If no breaking API changes have been made
within Zephyr and a different release, the port may work there as well

- [v2.0](https://github.com/zephyrproject-rtos/zephyr/tree/v2.0-branch)

## Integrating SDK

1. Depending on Zephyr version, apply .patch in release directory to Zephyr
   Kernel. Be sure to use the patch matching the Zephyr version in use.

```
$ cd $ZEPHYR_ROOT_DIR/
$ git apply $TICOS_SDK_ROOT/ports/zephyr/[v2.0]/zephyr-integration.patch
```

Note that `v2.2_v2.3/coredump-support.patch` should be applied for Zephyr 2.2
and 2.3 only.

2. Clone (or symlink) ticos-firmware-sdk in Zephyr Project

```
$ cd $ZEPHYR_ROOT_DIR/ext/lib/ticos
$ git clone https://github.com/ticos/ticos-firmware-sdk.git
```

3. Implement device-specific dependencies.

```
void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (sTicosDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-main",
    .software_version = "1.15.0",
    .hardware_version = "disco_l475_iot1",
  };
}
```

```
sTcsHttpClientConfig g_tcs_http_client_config = {
  .api_key = "<YOUR PROJECT KEY HERE>",
};
```

## Demo

An example integration and instructions can be found for the STM32L4 in
`$TICOS_SDK_ROOT/examples/zephyr/README.md`
