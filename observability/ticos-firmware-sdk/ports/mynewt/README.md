## Overview

To use the ticos-firmware-sdk with mynewt add following lines to
**project.yml**.

```yaml
repository.ticos-firmware-sdk:
  type: github
  vers: 0.0.0
  user: ticos
  repo: ticos-firmware-sdk
```

Adding this dependency will allow you to pull in Ticos features

```yaml
pkg.deps:
  - "@ticos-firmware-sdk/ports/mynewt"
```

You will also need to enable the following options in the **syscfg.vals:**
section of your `syscfg.yml` file:

```yaml
syscfg.vals:
[...]
    TICOS_ENABLE: 1
    TICOS_COREDUMP_CB: 1
    OS_COREDUMP: 1
    OS_COREDUMP_CB: 1
```

Other syscfgs you can configure can be found in [syscfg.yml](syscfg.yml)

Finally, on boot up initialize the Ticos subsystem from your `main` routine:

```c
#include "ticos/components.h"

int
main(int argc, char **argv)
{
// ...

    ticos_platform_boot();
// ...
}
```

## Using the GNU Build Id for Indexing

We recommend enabling the generation of a unique identifier for your project.
See https://ticos.io/symbol-file-build-ids for more details.

To enable, add the following compilation flags to your `pkg.yml`:

```
pkg.lflags:
  - -Wl,--build-id
pkg.cflags:
  - -DTICOS_USE_GNU_BUILD_ID=1
```

And add the following **after** the `.text` in your targets linker script:

```
    .note.gnu.build-id :
    {
        __start_gnu_build_id_start = .;
        KEEP(*(.note.gnu.build-id))
    } > FLASH
```

## Enabling Ticos Demo Shell Commands

The Ticos demo shell commands can be included in the mynewt shell (useful for
testing various Ticos features). To enable:

1. set `TICOS_CLI: 1` in the target's `syscfg.yml`.
2. enable the shell commands by calling `tcs_shell_init()` at the appropriate
   point in the application initialization (eg after `sysinit()`).
