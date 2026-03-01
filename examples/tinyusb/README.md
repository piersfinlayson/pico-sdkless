# pico-sdkless tinyusb Example

This example provides a complete, bare-metal, working tinyusb implementation for the RP2350 without using the Pico SDK.  It is intended to be used as a reference for building tinyusb-based firmware with pico-sdkless.

It implements a single CDC interface using the standard RP2350 VID/PID 2e8a/000f.  It sends "Hello pico-sdkless" until it receives some data, and then it echoes back whatever it receives.

## Usage

From this directory

```bash
make
```

This creates the following files

```text
build/pico-sl-tinyusb.bin
build/pico-sl-tinyusb.elf 
build/pico-sl-tinyusb.uf2
```

Flash your preferred format to a RP2350-based board and connect a terminal to the USB CDC port.  For example:

```bash
python3 -m serial.tools.miniterm --raw /dev/ttyACM0 115200
```

You should see "Hello pico-sdkless" repeatedly until you type something, at which point it will echo back what you typed.

```text
Hello pico-sdkless
Hello pico-sdkless
Hello pico-sdkless
# repeats until you type something
hello echo
```