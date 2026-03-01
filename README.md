# pico-sdkless

A minimal set of self-contained header files for the Raspberry Pi Pico RP2350 to build projects without including the full Pico SDK.  Only standard C headers and `arm-gcc-none-eabi` are required to use.

This project currently provides sufficient headers to build tinyusb for the RP2350 without the Pico SDK.  It is likely to be missing various other headers and definitions that may be required to build other projects, but contributions are welcome to expand the coverage of pico-sdkless.

## Usage

The easiest way to get going with pico-sdkless is to start from an [example](examples/README.md).  The examples also provide a basic reset handler, vector table, linker script, Makefile and libc implementation to get you up and running.  You can copy these files into your own project and modify as needed, or you can link against them from your Makefile.

Read on for instructions on how to create your own project with pico-sdkless.

### Include

You only need to include `pico.h` in your project.  A single C file in your project must define `PICO_SL_IMPLEMENTATION` before including `pico.h` to pull in the implementation code for any pico-sdkless functions and statics.

```c
#define PICO_SL_IMPLEMENTATION
#include "pico.h"
```

Various other stub headers are provided in case the code you are building needs them, but pico.h includes the entirety of the pico-sdkless implementation.

### IRQ support

If you require IRQ support and are using pico-sdkless's built-in IRQ handling implementation, you need to provide your own interrupt vector table and ensure that the `pico_sl_isr(irq_num)` function is called from the appropriate interrupt handlers.  For example:

```
void isr_usbctrl(void) {
    pico_sl_isr(USBCTRL_IRQ);
}
```

### Building

Instead of pointing your include path at the Pico SDK, point it here instead.

**Make**
```makefile
CFLAGS += -Ipath/to/pico-sdkless/include
```

**CMake**
```cmake
include_directories(path/to/pico-sdkless/include)
```

Then build and link your project as normal using the `arm-gcc-none-eabi` toolchain.  You will likely need to link against libc and libgcc.

### Options

There are some options you can configure with preprocessor defines:

```c
// Do not include pico-sdkless's panic() implementation and instead provide
// your own function `void panic(const char *fmt, ...)`.
#define PICO_SL_EXCLUDE_PANIC

// Do not include pico-sdkless's hard_assert() implementation and instead
// provide your own macro `#define hard_assert(x) ...`.
#define PICO_SL_EXCLUDE_HARD_ASSERT

// Do not include pico-sdkless's IRQ handling implementation and instead
// provide your own implementations of the functions `irq_add_shared_handler()`,
// `irq_remove_handler()`, and `irq_set_enabled()`.  `pico_sl_isr()` will also
// not be included.
#define PICO_SL_EXCLUDE_IRQ_HANDLING
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.