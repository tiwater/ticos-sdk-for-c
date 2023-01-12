## Ticos for Quantum Leaps' QP™

This folder contains an example integration of the Ticos SDK using the QP/C
port provided in `ports/qp`.

The demo was tested using the
[STM32F407 discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
but should work for any STM32F4xx based board.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

### Setup

_Note: these instructions reference the `$TICOS_SDK_ROOT` variable for
various paths. Either substitute the path to the Ticos SDK, or set it before
running the snippets, i.e.
`export TICOS_SDK_ROOT=absolute/path/to/ticos/sdk`._

1. Go into the demo app directory:

   ```bash
   $ cd $TICOS_SDK_ROOT/examples/qp/apps/ticos_demo_app
   ```

2. Clone the QP/C repo

   ```bash
   $ git clone git@github.com:QuantumLeaps/qpc.git --branch v6.6.0+ qpc
   ```

3. Apply patches to integrate Ticos SDK

   ```bash
   $ cd qpc
   $ patch include/qassert.h $TICOS_SDK_ROOT/ports/qp/qassert.h.patch
   $ patch src/qf_pkg.h $TICOS_SDK_ROOT/ports/qp/qf_pkg.h.patch
   ```

### Ticos Project Key

A Project Key will need to be baked into the
[demo app](https://ticos.io/demo-cli) to enable it to communicate with Ticos's
web services. Go to https://app.ticos.com/, navigate to the project you want
to use and select 'Settings'. Copy the 'Project API Key' and paste it into
`$TICOS_SDK_ROOT/examples/qp/apps/ticos_demo_app/src/main.c`, replacing
`<YOUR PROJECT KEY HERE>` with your Project Key.

### Building the App

After performing the steps above, run `make` from the demo app directory:

```bash
$ cd $TICOS_SDK_ROOT/examples/qp/apps/ticos_demo_app

$ EMBEDDED_Tcs_SDK_ROOT=$TICOS_SDK_ROOT QPC_DIR=qpc make
```

The target is built to:
`$TICOS_SDK_ROOT/examples/qp/apps/ticos_demo_app/build/ticos_demo_app.elf`

### Flashing the App

Run the `st-util` in one terminal:

```
$ st-util
st-util 1.5.1
2019-12-12T15:22:42 INFO common.c: Loading device parameters....
2019-12-12T15:22:42 INFO common.c: Device connected is: F4 device, id 0x10076413
[...]
```

Then flash the .elf using GDB:

```
$ arm-none-eabi-gdb --eval-command="target remote localhost:4242" --ex="mon reset" --ex="load" --ex="mon reset" \
  --se=$TICOS_SDK_ROOT/examples/qp/apps/ticos_demo_app/build/ticos_demo_app.elf --batch
Generating /path/to/ticos-sdk/examples/qp/apps/ticos_demo_app/build/ticos_demo_app.elf
```

### Running the App

The USART2 peripheral of the board is used as a console/shell.

Follow the instructions in the
[user manual](https://www.st.com/content/ccc/resource/technical/document/user_manual/70/fe/4a/3f/e7/e1/4f/7d/DM00039084.pdf/files/DM00039084.pdf/jcr:content/translations/en.DM00039084.pdf)
(see section 6.1.3 "ST-LINK/V2-A VCP configuration") to connect the RX/TX to
either the on-board or an external USB-to-serial adapter.

Then use any serial terminal program to connect to it:

```
$ miniterm.py --raw /dev/cu.usbserial* 115200
--- Miniterm on /dev/cu.usbserial-1420  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
Ticos QP demo app started...
tcs> help
get_core: Get coredump info
clear_core: Clear an existing coredump
print_chunk: Get next Ticos data chunk to send and print as a curl command
crash: Trigger a crash
get_device_info: Get device info
help: Lists all commands
```

### Causing a crash

Detach the debugger now and hard-reset the board. Otherwise, if the debugger is
still attached while crashing, the demo application will pause at a breakpoint
instruction.

Command `test_hardfault` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`:

```
tcs> tcs test_hardfault
Ticos QP demo app started...

tcs>
```

Upon crashing, a coredump will be written to noinit RAM. After that the demo app
will restart immediately.

To check whether the coredump has been captured, try running the `get_core`
command after the device reboots:

```
tcs> get_core
Has coredump with size: 584
```

This confirms that a coredump of 584 bytes has been captured.

### Posting Ticos Data for Analysis

#### Uploading Symbols

Ticos needs the symbols for the firmware in order to analyze the coredump.
The ELF is located at `build/ticos_demo_app.elf`. This .elf contains the
symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://ticos.io/symbol-file-build-ids).

#### Posting captured data to Ticos

The STM32F407 board does not have the capability to connect to the internet
directly. Therefore, for debug purposes, the messages to push to the Ticos
cloud can also be dumped from the CLI using the `print_chunk` command:

```
tcs> print_chunk
echo \
[...]
| xxd -p -r | curl -X POST https://chunks.ticos.com/api/v0/chunks/DEMOSERIAL\
 -H 'Ticos-Project-Key:<YOUR_PROJECT_KEY>\
 -H 'Content-Type:application/octet-stream' --data-binary @- -i
```

You can copy and paste the output into a terminal to upload the captured data to
Ticos.
