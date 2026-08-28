# Changelog

pico-sdkless follows [semantic versioning](https://semver.org).  Releases
before 1.0.0 may change the API in a minor version.

## 0.1.1 - 2026-08-28

- The XIP QMI registers: `XIP_QMI_M0_TIMING` and its clock divider fields.
- A function can be placed in RAM.  The example linker script gives `.ramfunc`
  an output section and the example reset handler copies it out of flash.
- `rp2040_chip_version` returns 2.  It exists for tinyusb, which asks an
  RP2040 question, and reading the RP2350's chip ID revision answered
  something else.
- `USB_MAX_ENDPOINTS` can be defined by the application, so it need not
  allocate for all 16.  The USB DPRAM layout keeps the hardware's own count.
- More clock definitions: the ROSC divider, the `clk_sys` sources and
  selections, the `clk_usb` divider, `clk_ref`'s divider and the PLL_SYS reset
  bit.
