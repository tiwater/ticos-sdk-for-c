# FreeRTOS Specific Port

## Overview

This directory contains an implementation of Ticos dependency functions when
a platform is built on top of [FreeRTOS](https://www.freertos.org/).

## Usage

1. Add `$TICOS_FIRMWARE_SDK/ports/freertos/include` to your projects include
   path
2. Prior to using the Ticos SDK in your code, initialize the FreeRTOS port.

```c
#include "ticos/ports/freertos.h"

void main(void) {

  ticos_freertos_port_boot();
}
```

## Heap Tracking

The ticos-firmware-sdk has a built in utility for tracking heap allocations to facilitate debug
of out of memory bugs. To enable, add the following to your `ticos_platform_config.h` file:

```c
#define TICOS_FREERTOS_PORT_HEAP_STATS_ENABLE 1
#define TICOS_COREDUMP_HEAP_STATS_LOCK_ENABLE 0
#define TICOS_COREDUMP_COLLECT_HEAP_STATS 1
```
