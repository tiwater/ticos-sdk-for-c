# nRF-Connect Ticos Example

This is a small example application showing a Ticos integration running on an
nrf52840 development board, using the nRF Connect SDK. Any nRF board should also
work. The example has been tested on:

- nRF52840-DK
- nRF5340-DK

## Usage

Make sure you have the Zephyr / nRF-Connect tools installed first:

<https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.0.2/nrf/gs_installing.html>

To build and flash this example to an nRF52840-DK (PCA10056), run the following
commands:

```bash
❯ west init --local ticos_demo_app
❯ west update
❯ west build --board nrf52840dk_nrf52840 ticos_demo_app
❯ west flash
```

Open a serial terminal to access the console:

```bash
# for example, pypserial-miniterm
❯ pyserial-miniterm --raw /dev/ttyACM0 115200
```

The console has several Ticos test commands available:

```bash
uart:~$ tcs help
Subcommands:
  clear_core          :clear coredump collected
  export              :dump chunks collected by Ticos SDK using
                       https://ticos.io/chunk-data-export
  get_core            :check if coredump is stored and present
  get_device_info     :display device information
  get_latest_release  :checks to see if new ota payload is available
  post_chunks         :Post Ticos data to cloud
  test                :commands to verify ticos data collection
                       (https://ticos.io/mcu-test-commands)
  post_chunks         :Post Ticos data to cloud
  get_latest_release  :checks to see if new ota payload is available
```

The `tcs test` subgroup contains commands for testing Ticos functionality:

```bash
uart:~$ tcs test help
test - commands to verify ticos data collection
       (https://ticos.io/mcu-test-commands)
Subcommands:
  assert       :trigger ticos assert
  busfault     :trigger a busfault
  hang         :trigger a hang
  hardfault    :trigger a hardfault
  memmanage    :trigger a memory management fault
  usagefault   :trigger a usage fault
  zassert      :trigger a zephyr assert
  reboot       :trigger a reboot and record it using ticos
  heartbeat    :trigger an immediate capture of all heartbeat metrics
  log_capture  :trigger capture of current log buffer contents
  logs         :writes test logs to log buffer
  trace        :capture an example trace event
```

For example, to test the coredump functionality:

1. run `tcs test hardfault` and wait for the board to reset
2. run `tcs get_core` to confirm the coredump was saved
3. run `tcs export` to print out the base-64 chunks:

   ```plaintext
   uart:~$ tcs export
   <inf> <tcs>: MC:SE4DpwIEAwEKbW5yZjUyX2V4YW1wbGUJZTAuMC4xBmFhC0Z5RE1gF8EEhgFpSW5mbyBsb2chAmxXYXJuaW5nIGxvZyEDakVycm9yIGxvZyE=:
   <inf> <tcs>: MC:gE6A/A==:
   ```

4. upload the chunks to Ticos. see here for details:

   <https://ticos.io/chunks-debug>
