[![CircleCI](https://circleci.com/gh/ticos/ticos-firmware-sdk.svg?style=svg)](https://circleci.com/gh/ticos/ticos-firmware-sdk)
[![Coverage](https://img.shields.io/codecov/c/gh/ticos/ticos-firmware-sdk/master)](https://codecov.io/gh/ticos/ticos-firmware-sdk/)

# Ticos Firmware SDK

Ship Firmware with Confidence.

More details about the Ticos platform itself, how it works, and step-by-step
integration guides
[can be found here](https://ticos.io/embedded-getting-started).

# Getting Started

To start integrating in your platform today,
[create a Ticos cloud account](https://ticos.io/signup).

# Components

The SDK is designed as a collection of components, so you can include only what
is needed for your project. The SDK has been designed to have minimal impact on
code-space, bandwidth, and power consumption.

The [`components`](components/) directory folder contains the various components
of the SDK. Each component contains a `README.md`, source code, header files and
"platform" header files.

The platform header files describe the interfaces which the component relies on
that you must implement.

For some of the platform dependencies we have provided ports that can be linked
into your system directly, or used as a template for further customization. You
can find them in the [`ports`](ports/) folder.

For some of the popular MCUs & vendor SDKs, we have already provided a reference
implementation for platform dependencies which can be found in the
[`examples`](examples/) folder. These can also serve as a good example when
initially setting up the SDK on your platform.

### Main components

- `panics` – fault handling, coredump and reboot tracking and reboot loop
  detection API.
- `metrics` - used to monitor device health over time (i.e. connectivity,
  battery life, MCU resource utilization, hardware degradation, etc.)

Please refer to the `README.md` in each of these for more details.

### Support components

- `core` – common code that is used by all other components.
- `demo` - common code that is used by demo apps for the various platforms.
- `http` – http client API, to post coredumps and events directly to the
  Ticos service from devices.
- `util` – various utilities.

# Integrating the Ticos SDK

## Add Ticos SDK to Your Repository

The Ticos SDK can be added directly into your repository. The structure
typically looks like:

```
<YOUR_PROJECT>
├── third_party/ticos
│               ├── ticos-firmware-sdk (submodule)
│               │
│               │ # Files where port to your platform will be implemented
│               ├── ticos_platform_port.c
│               ├── ticos_platform_coredump_regions.c
│               │
│               │ # Configuration headers
│               ├── ticos_platform_config.h
│               ├── ticos_trace_reason_user_config.def
│               ├── ticos_metrics_heartbeat_config.def
│               └── ticos_platform_log_config.h
```

If you are using `git`, the Ticos SDK is typically added to a project as a
submodule:

```
$ git submodule add git@github.com:ticos/ticos-firmware-sdk.git $YOUR_PROJECT/third_party/ticos/ticos-firmware-sdk
```

This makes it easy to track the history of the Ticos SDK. You should not need
to make modifications to the Ticos SDK. The typical update flow is:

- `git pull` the latest upstream
- check [CHANGES.md](CHANGES.md) to see if any modifications are needed
- update to the new submodule commit in your repo.

Alternatively, the Ticos SDK may be added to a project as a git subtree or by
copying the source into a project.

## Add sources to Build System

### Make

If you are using `make`, `makefiles/TicosWorker.mk` can be used to very
easily collect the source files and include paths required by the SDK.

```c
TICOS_SDK_ROOT := <The to the root of this repo from your project>
TICOS_COMPONENTS := <The SDK components to be used, i.e "core util">
include $(TICOS_SDK_ROOT)/makefiles/TicosWorker.mk
<YOUR_SRC_FILES> += $(TICOS_COMPONENTS_SRCS)
<YOUR_INCLUDE_PATHS> += $(TICOS_COMPONENTS_INC_FOLDERS)
```

### Cmake

If you are using `cmake`, `cmake/Ticos.cmake` in a similar fashion to
collection source files and include paths:

```c
set(TICOS_SDK_ROOT <The path to the root of the ticos-firmware-sdk repo>)
list(APPEND TICOS_COMPONENTS <The SDK components to be used, i.e "core util">)
include(${TICOS_SDK_ROOT}/cmake/Ticos.cmake)
ticos_library(${TICOS_SDK_ROOT} TICOS_COMPONENTS
 TICOS_COMPONENTS_SRCS TICOS_COMPONENTS_INC_FOLDERS)

# ${TICOS_COMPONENTS_SRCS} contains the sources
# needed for the library and ${TICOS_COMPONENTS_INC_FOLDERS} contains the include paths
```

### Other Build Systems

If you are not using one of the above build systems, to include the SDK you need
to do is:

- Add the `.c` files located at `components/<component>/src/*.c` to your build
  system
- Add `components/<component>/include` to the include paths you pass to the
  compiler

#### Running the unit tests

The SDK code is covered extensively by unit tests. They can be found in the
`tests/` folder. If you'd like to run them yourself, check out the instructions
in [tests/README.md](tests/README.md).

To learn more about unit testing best practices for firmware development, check
out
[our blog post on this topic](https://interrupt.ticos.com/blog/unit-testing-basics)!

The unit tests are run by CircleCI upon every commit to this repo. See badges at
the top for build & test coverage status of the `master` branch.

# FAQ

- Why does a coredump not show up under "Issues" after uploading it?

  - Make sure to upload the symbols to the same project to which you upload
    coredumps. Also make sure the software type and software version reported by
    the device (see "Device information" in `components/core/README.md`) match
    the software type and software version that was entered when creating the
    Software Version and symbol artifact online.
    [More information on Build Ids and uploading Symbol Files can be found here](https://ticos.io/symbol-file-build-ids).

- I'm getting error XYZ, what to do now?

  - Don't hesitate to contact us for help! You can reach us through
    [support@ticos.com](mailto:support@ticos.com).

# License

Unless specifically indicated otherwise in a file, all ticos-firmware-sdk
files are all licensed under the [Ticos License](/License.txt). (A few files
in the [examples](/examples) and [ports](/ports) directory are licensed
differently based on vendor requirements.)
