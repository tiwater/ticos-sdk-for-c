# Quantum Leaps' QP™ Specific Port

## Overview

This directory contains patches needed to integrate the Ticos SDK into
Quantum Leaps' QP™.

The patches have been verified against versions v6.0.0 up to v6.6.0 of QP/C and
QP/C++.

The instructions below assume you have an environment already setup for building
and flashing a QP application. If you do not, see the
[official site](https://www.state-machine.com) for instructions.

## Integrating SDK

1. Apply `qassert.h.patch`

QP uses its `Q_ASSERT...` macros to ensure the QP API is used correctly and the
system is behaving correctly. The `qassert.h.patch` change these macros such
that failing assertions will trigger the Ticos SDK to capture a coredump.

```
$ cd $QP_ROOT/
$ patch include/qassert.h $TICOS_SDK_ROOT/ports/qp/qassert.h.patch
```

2. Apply `qf_pkg.h.patch` or `qf_pkg.hpp.patch`

This patches a second set of assertion macros that QP uses internally, since
version 6.3.2. _If you are using an older version than 6.3.2, you can skip this
step._

In case you are using QP/C:

```
$ cd $QP_ROOT/
$ patch src/qf_pkg.h $TICOS_SDK_ROOT/ports/qp/qf_pkg.h.patch
```

In case you are using QP/C++:

```
$ cd $QP_ROOT/
$ patch src/qf_pkg.hpp $TICOS_SDK_ROOT/ports/qp/qf_pkg.hpp.patch
```

3. Remove `Q_onAssert`

After applying the `ports/qp/qassert.h.patch` and `ports/qp/qf_pkg.h.patch`
files from the Ticos Firmware SDK, the `Q_onAssert` function will no longer
be necessary. Instead, Ticos's assertion handling will be used.

In case your application's code is using `Q_onAssert` directly, either replace
the usages with the `Q_ASSERT...` macros or use Ticos's `TICOS_ASSERT...`
macros.

4. Implement device-specific dependencies.

```
void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (sTicosDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "qp-main",
    .software_version = "1.15.0",
    .hardware_version = "stm32f407g-disc1",
  };
}
```

```
sTcsHttpClientConfig g_tcs_http_client_config = {
  .api_key = "<YOUR PROJECT KEY HERE>",
};
```

## Demo

An example integration and instructions can be found for the STM32F407 in
`$TICOS_SDK_ROOT/examples/qp/README.md`
