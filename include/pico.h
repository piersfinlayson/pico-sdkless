// Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
//
// MIT License

// A replacement pico.h header to avoid needing the Pico SDK

#if !defined(PICO_H)
#define PICO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

//--------------------------------------------------------------------+
// Platform identification
//--------------------------------------------------------------------+
#define PICO_RP2350 1

//--------------------------------------------------------------------+
// Basic IO types (from hardware/address_mapped.h in the SDK)
//--------------------------------------------------------------------+
typedef volatile uint32_t io_rw_32;
typedef const volatile uint32_t io_ro_32;
typedef volatile uint32_t io_wo_32;
typedef volatile uint16_t io_rw_16;
typedef volatile uint8_t  io_rw_8;

// uint is used pervasively in the SDK
typedef unsigned int uint;

//--------------------------------------------------------------------+
// Compiler attributes
// Guarded: newlib's cdefs.h defines several of these already
//--------------------------------------------------------------------+
#ifndef __unused
#define __unused        __attribute__((unused))
#endif

#ifndef __always_inline
#define __always_inline __attribute__((always_inline))
#endif

#ifndef __noinline
#define __noinline      __attribute__((noinline))
#endif

#ifndef __packed
#define __packed        __attribute__((packed))
#endif

#ifndef __aligned
#define __aligned(x)    __attribute__((aligned(x)))
#endif

// Stubbed: not placing functions in RAM
#define __no_inline_not_in_flash_func(x) x
#define __not_in_flash(x)

//--------------------------------------------------------------------+
// Atomic hardware access aliases
// RP2350 - each peripheral register block has three aliases:
//   +0x1000  XOR
//   +0x2000  SET (atomic bitwise OR)
//   +0x3000  CLEAR (atomic bitwise AND NOT)
//--------------------------------------------------------------------+
#define hw_xor_alias_untyped(addr)   ((void *)((uintptr_t)(addr) + 0x1000u))
#define hw_set_alias_untyped(addr)   ((void *)((uintptr_t)(addr) + 0x2000u))
#define hw_clear_alias_untyped(addr) ((void *)((uintptr_t)(addr) + 0x3000u))

//--------------------------------------------------------------------+
// Bit manipulation
//--------------------------------------------------------------------+
#ifndef TU_BIT
#define TU_BIT(n)  (1UL << (n))
#endif

//--------------------------------------------------------------------+
// Volatile cast removal
// Used when passing DPRAM pointers to memcpy-like functions
//--------------------------------------------------------------------+
#define remove_volatile_cast(t, x) ((t)(uintptr_t)(x))

//--------------------------------------------------------------------+
// Assert / panic
//--------------------------------------------------------------------+
#if !defined(SL_EXCLUDE_PANIC)
__attribute__((always_inline)) inline void panic(const char *fmt, ...) {
    (void)fmt;
    __asm volatile ("bkpt #0");
    for (;;);  // suppress noreturn warning
}
#else // SL_EXCLUDE_PANIC
// panic() is provided separately
extern void panic(const char *fmt, ...);
#endif // SL_EXCLUDE_PANIC

#if !defined(SL_EXCLUDE_HARD_ASSERT)
#define hard_assert(x) \
    do { if (!(x)) panic("hard_assert failed: " #x " at %s:%d", __FILE__, __LINE__); } while(0)
#else // SL_EXCLUDE_HARD_ASSERT
// hard_assert() is provided separately
#endif // SL_EXCLUDE_HARD_ASSERT

//--------------------------------------------------------------------+
// SYSINFO - chip revision
// SYSINFO_CHIP_ID[7:4] = REVISION, 0=B0, 1=B1, 2=B2
// SDK adds 1 so callers see B0->1, B1->2, B2->3
// The E2 errata guard in dcd_rp2040.c uses >= 2, i.e. requires B1+
//--------------------------------------------------------------------+
#define SYSINFO_BASE                        0x40000000u
#define SYSINFO_CHIP_ID_OFFSET              0x00u
#define SYSINFO_CHIP_ID_MANUFACTURER_BITS   0x00000fffu
#define SYSINFO_CHIP_ID_PART_LSB            12u
#define SYSINFO_CHIP_ID_PART_BITS           0x0ffff000u
#define SYSINFO_CHIP_ID_REVISION_LSB        28u
#define SYSINFO_CHIP_ID_REVISION_BITS       0xf0000000u

#define MANUFACTURER_RPI    0x927u
#define PART_RP2            0x2u

static inline uint8_t rp2040_chip_version(void) {
    uint32_t chip_id = *((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));
    uint32_t __unused manufacturer = chip_id & SYSINFO_CHIP_ID_MANUFACTURER_BITS;
    uint32_t __unused part = (chip_id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    assert(manufacturer == MANUFACTURER_RPI);
    assert(part == PART_RP2);
    uint version = (chip_id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    return (uint8_t)version;
}

//--------------------------------------------------------------------+
// Busy wait
// The USB SIE_CTRL / BUF_CTRL concurrent access requirementneeds at
// least 12 cycles @ 48 MHz USB clock.
//
// 3-cycle loop: subs + bne on Cortex-M0+
//--------------------------------------------------------------------+
static inline void busy_wait_at_least_cycles(uint32_t cycles) {
    uint32_t count = (cycles + 2u) / 3u;
    __asm volatile (
        "1: subs %0, %0, #1\n"
        "   bne  1b\n"
        : "+r" (count) : : "cc"
    );
}

//--------------------------------------------------------------------+
// USB endpoint / buffer count constants
//--------------------------------------------------------------------+
#ifndef USB_MAX_ENDPOINTS
#define USB_MAX_ENDPOINTS               16u
#endif

#ifndef USB_HOST_INTERRUPT_ENDPOINTS
#define USB_HOST_INTERRUPT_ENDPOINTS    15u
#endif

//--------------------------------------------------------------------+
// USB base addresses
//--------------------------------------------------------------------+
#define USBCTRL_DPRAM_BASE  0x50100000u
#define USBCTRL_REGS_BASE   0x50110000u

//--------------------------------------------------------------------+
// DPRAM structures - device mode
//
// Layout:
//   0x000  setup_packet[8]
//   0x008  ep_ctrl[15]        15 * (in:u32 + out:u32) = 0x078
//   0x080  ep_buf_ctrl[16]    16 * (in:u32 + out:u32) = 0x080
//   0x100  ep0_buf_a[0x40]
//   0x140  ep0_buf_b[0x40]
//   0x180  epx_data[0xe80]    remainder of 4K DPRAM
//   0x1000 end
//--------------------------------------------------------------------+
typedef struct {
    io_rw_32 in;
    io_rw_32 out;
} ep_ctrl_entry_t;

typedef struct {
    io_rw_32 in;
    io_rw_32 out;
} ep_buf_ctrl_entry_t;

typedef struct {
    uint8_t               setup_packet[8];                    // 0x000
    ep_ctrl_entry_t       ep_ctrl[USB_MAX_ENDPOINTS - 1];     // 0x008
    ep_buf_ctrl_entry_t   ep_buf_ctrl[USB_MAX_ENDPOINTS];     // 0x080
    uint8_t               ep0_buf_a[0x40];                    // 0x100
    uint8_t               ep0_buf_b[0x40];                    // 0x140
    uint8_t               epx_data[0xe80];                    // 0x180
} usb_device_dpram_t;

static_assert(offsetof(usb_device_dpram_t, ep_ctrl)     == 0x008, "ep_ctrl offset");
static_assert(offsetof(usb_device_dpram_t, ep_buf_ctrl) == 0x080, "ep_buf_ctrl offset");
static_assert(offsetof(usb_device_dpram_t, ep0_buf_a)   == 0x100, "ep0_buf_a offset");
static_assert(offsetof(usb_device_dpram_t, ep0_buf_b)   == 0x140, "ep0_buf_b offset");
static_assert(offsetof(usb_device_dpram_t, epx_data)    == 0x180, "epx_data offset");
static_assert(sizeof(usb_device_dpram_t)                == 0x1000, "DPRAM device size");

#define usb_dpram ((usb_device_dpram_t *)USBCTRL_DPRAM_BASE)

//--------------------------------------------------------------------+
// DPRAM structures - host mode
//--------------------------------------------------------------------+
typedef struct {
    io_rw_32 ctrl;
} usbh_int_ep_ctrl_entry_t;

typedef struct {
    io_rw_32 ctrl;
} usbh_int_ep_buf_ctrl_entry_t;

typedef struct {
    uint8_t  setup_packet[8];                                               // 0x000
    io_rw_32 epx_ctrl;                                                      // 0x008
    io_rw_32 epx_buf_ctrl;                                                  // 0x00c
    uint8_t  epx_data[0x140];                                               // 0x010
    usbh_int_ep_ctrl_entry_t     int_ep_ctrl[USB_HOST_INTERRUPT_ENDPOINTS];
    usbh_int_ep_buf_ctrl_entry_t int_ep_buffer_ctrl[USB_HOST_INTERRUPT_ENDPOINTS];
} usb_host_dpram_t;

#define usbh_dpram ((usb_host_dpram_t *)USBCTRL_DPRAM_BASE)

//--------------------------------------------------------------------+
// USB hardware register block
//
//   0x000  ADDR_ENDP
//   0x004  ADDR_ENDP1  )
//   ...                )  interrupt endpoint addr regs, host mode
//   0x03c  ADDR_ENDP15 )
//   0x040  MAIN_CTRL
//   0x044  SOF_WR
//   0x048  SOF_RD
//   0x04c  SIE_CTRL
//   0x050  SIE_STATUS
//   0x054  INT_EP_CTRL
//   0x058  BUFF_STATUS
//   0x05c  BUFF_CPU_SHOULD_HANDLE
//   0x060  ABORT
//   0x064  ABORT_DONE
//   0x068  EP_STALL_ARM
//   0x06c  NAK_POLL
//   0x070  EP_STATUS_STALL_NAK
//   0x074  USB_MUXING
//   0x078  USB_PWR
//   0x07c  USBPHY_DIRECT
//   0x080  USBPHY_DIRECT_OVERRIDE
//   0x084  USBPHY_TRIM
//   0x088  (reserved)
//   0x08c  INTR
//   0x090  INTE
//   0x094  INTF
//   0x098  INTS
//--------------------------------------------------------------------+
typedef struct {
    io_rw_32 addr_endp;              // 0x000
    io_rw_32 addr_endp1;             // 0x004
    io_rw_32 addr_endp2;             // 0x008
    io_rw_32 addr_endp3;             // 0x00c
    io_rw_32 addr_endp4;             // 0x010
    io_rw_32 addr_endp5;             // 0x014
    io_rw_32 addr_endp6;             // 0x018
    io_rw_32 addr_endp7;             // 0x01c
    io_rw_32 addr_endp8;             // 0x020
    io_rw_32 addr_endp9;             // 0x024
    io_rw_32 addr_endp10;            // 0x028
    io_rw_32 addr_endp11;            // 0x02c
    io_rw_32 addr_endp12;            // 0x030
    io_rw_32 addr_endp13;            // 0x034
    io_rw_32 addr_endp14;            // 0x038
    io_rw_32 addr_endp15;            // 0x03c
    io_rw_32 main_ctrl;              // 0x040
    io_rw_32 sof_wr;                 // 0x044
    io_rw_32 sof_rd;                 // 0x048
    io_rw_32 sie_ctrl;               // 0x04c
    io_rw_32 sie_status;             // 0x050
    io_rw_32 int_ep_ctrl;            // 0x054
    io_rw_32 buf_status;             // 0x058
    io_rw_32 buf_cpu_should_handle;  // 0x05c
    io_rw_32 abort;                  // 0x060
    io_rw_32 abort_done;             // 0x064
    io_rw_32 ep_stall_arm;           // 0x068
    io_rw_32 nak_poll;               // 0x06c
    io_rw_32 ep_status_stall_nak;    // 0x070
    io_rw_32 muxing;                 // 0x074
    io_rw_32 pwr;                    // 0x078
    io_rw_32 usbphy_direct;          // 0x07c
    io_rw_32 usbphy_direct_override; // 0x080
    io_rw_32 usbphy_trim;            // 0x084
    io_rw_32 _pad0;                  // 0x088
    io_rw_32 intr;                   // 0x08c
    io_rw_32 inte;                   // 0x090
    io_rw_32 intf;                   // 0x094
    io_rw_32 ints;                   // 0x098
} usb_hw_t;

// addr_endp1 is the base of the interrupt endpoint address control array,
// accessed as usb_hw->int_ep_addr_ctrl[n] in hcd_rp2040.c
#define int_ep_addr_ctrl addr_endp1
#define dev_addr_ctrl    addr_endp

static_assert(offsetof(usb_hw_t, main_ctrl)  == 0x040, "main_ctrl offset");
static_assert(offsetof(usb_hw_t, sie_ctrl)   == 0x04c, "sie_ctrl offset");
static_assert(offsetof(usb_hw_t, buf_status) == 0x058, "buf_status offset");
static_assert(offsetof(usb_hw_t, inte)       == 0x090, "inte offset");
static_assert(offsetof(usb_hw_t, ints)       == 0x098, "ints offset");

#define usb_hw ((usb_hw_t *)USBCTRL_REGS_BASE)

//--------------------------------------------------------------------+
// USB register bit definitions
//--------------------------------------------------------------------+

// MAIN_CTRL
#define USB_MAIN_CTRL_CONTROLLER_EN_BITS        (1u << 0)
#define USB_MAIN_CTRL_HOST_NDEVICE_BITS         (1u << 1)
#define USB_MAIN_CTRL_SIM_TIMING_BITS           (1u << 31)

// SIE_CTRL
#define USB_SIE_CTRL_START_TRANS_BITS           (1u << 0)
#define USB_SIE_CTRL_SEND_SETUP_BITS            (1u << 1)
#define USB_SIE_CTRL_SEND_DATA_BITS             (1u << 2)
#define USB_SIE_CTRL_RECEIVE_DATA_BITS          (1u << 3)
#define USB_SIE_CTRL_STOP_TRANS_BITS            (1u << 4)
#define USB_SIE_CTRL_PREAMBLE_EN_BITS           (1u << 6)
#define USB_SIE_CTRL_SOF_SYNC_BITS              (1u << 8)
#define USB_SIE_CTRL_SOF_EN_BITS                (1u << 9)
#define USB_SIE_CTRL_KEEP_ALIVE_EN_BITS         (1u << 10)
#define USB_SIE_CTRL_VBUS_EN_BITS               (1u << 11)
#define USB_SIE_CTRL_RESUME_BITS                (1u << 12)
#define USB_SIE_CTRL_RESET_BUS_BITS             (1u << 13)
#define USB_SIE_CTRL_PULLUP_EN_BITS             (1u << 16)
#define USB_SIE_CTRL_PULLDOWN_EN_BITS           (1u << 15)
#define USB_SIE_CTRL_TRANSCEIVER_PD_BITS        (1u << 18)
#define USB_SIE_CTRL_EP0_INT_1BUF_BITS          (1u << 29)
#define USB_SIE_CTRL_EP0_INT_2BUF_BITS          (1u << 30)

// SIE_STATUS
#define USB_SIE_STATUS_VBUS_DETECTED_BITS       (1u << 0)
#define USB_SIE_STATUS_LINE_STATE_BITS          (3u << 2)
#define USB_SIE_STATUS_SUSPENDED_BITS           (1u << 4)
#define USB_SIE_STATUS_SPEED_BITS               (3u << 8)
#define USB_SIE_STATUS_SPEED_LSB                8u
#define USB_SIE_STATUS_VBUS_OVER_CURR_BITS      (1u << 10)
#define USB_SIE_STATUS_RESUME_BITS              (1u << 11)
#define USB_SIE_STATUS_CONNECTED_BITS           (1u << 16)
#define USB_SIE_STATUS_SETUP_REC_BITS           (1u << 17)
#define USB_SIE_STATUS_TRANS_COMPLETE_BITS      (1u << 18)
#define USB_SIE_STATUS_BUS_RESET_BITS           (1u << 19)
#define USB_SIE_STATUS_CRC_ERROR_BITS           (1u << 24)
#define USB_SIE_STATUS_BIT_STUFF_ERROR_BITS     (1u << 25)
#define USB_SIE_STATUS_RX_OVERFLOW_BITS         (1u << 26)
#define USB_SIE_STATUS_RX_TIMEOUT_BITS          (1u << 27)
#define USB_SIE_STATUS_NAK_REC_BITS             (1u << 28)
#define USB_SIE_STATUS_STALL_REC_BITS           (1u << 29)
#define USB_SIE_STATUS_ACK_REC_BITS             (1u << 30)
#define USB_SIE_STATUS_DATA_SEQ_ERROR_BITS      (1u << 31)

// ADDR_ENDP (EPX)
#define USB_ADDR_ENDP_ADDRESS_BITS              (0x7fu << 0)
#define USB_ADDR_ENDP_ENDPOINT_LSB              16u
#define USB_ADDR_ENDP_ENDPOINT_BITS             (0xfu << 16)

// ADDR_ENDP1..15 (interrupt endpoints, host mode)
#define USB_ADDR_ENDP1_ADDRESS_BITS             (0x7fu << 0)
#define USB_ADDR_ENDP1_ENDPOINT_LSB             16u
#define USB_ADDR_ENDP1_ENDPOINT_BITS            (0xfu << 16)
#define USB_ADDR_ENDP1_INTEP_DIR_BITS           (1u << 25)
#define USB_ADDR_ENDP1_INTEP_PREAMBLE_BITS      (1u << 26)

// INTE / INTS / INTF / INTR
#define USB_INTS_HOST_CONN_DIS_BITS             (1u << 0)
#define USB_INTS_HOST_RESUME_BITS               (1u << 1)
#define USB_INTS_HOST_SOF_BITS                  (1u << 2)
#define USB_INTS_TRANS_COMPLETE_BITS            (1u << 3)
#define USB_INTS_BUFF_STATUS_BITS               (1u << 4)
#define USB_INTS_ERROR_DATA_SEQ_BITS            (1u << 5)
#define USB_INTS_ERROR_RX_TIMEOUT_BITS          (1u << 6)
#define USB_INTS_ERROR_RX_OVERFLOW_BITS         (1u << 7)
#define USB_INTS_ERROR_BIT_STUFF_BITS           (1u << 8)
#define USB_INTS_ERROR_CRC_BITS                 (1u << 9)
#define USB_INTS_STALL_BITS                     (1u << 10)
#define USB_INTS_VBUS_DETECT_BITS               (1u << 11)
#define USB_INTS_BUS_RESET_BITS                 (1u << 12)
#define USB_INTS_DEV_CONN_DIS_BITS              (1u << 13)
#define USB_INTS_DEV_SUSPEND_BITS               (1u << 14)
#define USB_INTS_DEV_RESUME_FROM_HOST_BITS      (1u << 15)
#define USB_INTS_SETUP_REQ_BITS                 (1u << 16)
#define USB_INTS_DEV_SOF_BITS                   (1u << 17)

// dcd_rp2040.c references this via the INTF register name
#define USB_INTF_DEV_SOF_BITS                   USB_INTS_DEV_SOF_BITS

// EP_STALL_ARM
#define USB_EP_STALL_ARM_EP0_IN_BITS            (1u << 0)
#define USB_EP_STALL_ARM_EP0_OUT_BITS           (1u << 1)

// USB_MUXING (usb_hw->muxing)
#define USB_USB_MUXING_TO_PHY_BITS              (1u << 0)
#define USB_USB_MUXING_SOFTCON_BITS             (1u << 3)

// USB_PWR (usb_hw->pwr)
#define USB_USB_PWR_VBUS_DETECT_BITS             (1u << 2)
#define USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS (1u << 3)

// SOF_RD
#define USB_SOF_RD_BITS                         0x7ffu

// Buffer control register bits
// Each ep_buf_ctrl register holds two 16-bit buffer control fields:
//   bits [15:0]  = buffer 0
//   bits [31:16] = buffer 1 (shifted up by 16 by prepare_ep_buffer when buf_id=1)
#define USB_BUF_CTRL_LEN_MASK                   (0x3ffu)
#define USB_BUF_CTRL_AVAIL                      (1u << 10)
#define USB_BUF_CTRL_STALL                      (1u << 11)
#define USB_BUF_CTRL_SEL                        (1u << 12)
#define USB_BUF_CTRL_DATA0_PID                  (0u)
#define USB_BUF_CTRL_DATA1_PID                  (1u << 13)
#define USB_BUF_CTRL_LAST                       (1u << 14)
#define USB_BUF_CTRL_FULL                       (1u << 15)

// Endpoint control register bits
#define EP_CTRL_ENABLE_BITS                     (1u << 31)
#define EP_CTRL_DOUBLE_BUFFERED_BITS            (1u << 30)
#define EP_CTRL_INTERRUPT_PER_BUFFER            (1u << 29)
#define EP_CTRL_INTERRUPT_PER_DOUBLE_BUFFER     (1u << 28)
#define EP_CTRL_INTERRUPT_ON_NAK                (1u << 16)
#define EP_CTRL_INTERRUPT_ON_STALL              (1u << 17)
#define EP_CTRL_BUFFER_TYPE_LSB                 26u
#define EP_CTRL_HOST_INTERRUPT_INTERVAL_LSB     16u

//--------------------------------------------------------------------+
// Clocks peripheral
//--------------------------------------------------------------------+
#define CLOCKS_BASE         0x40010000
#define CLOCK_CLK_GPOUT0_CTRL   (*((volatile uint32_t *)(CLOCKS_BASE + 0x00)))
#define CLOCK_CLK_GPOUT0_DIV    (*((volatile uint32_t *)(CLOCKS_BASE + 0x04)))
#define CLOCK_CLK_GPOUT0_SEL    (*((volatile uint32_t *)(CLOCKS_BASE + 0x08)))
#define CLOCK_REF_CTRL          (*((volatile uint32_t *)(CLOCKS_BASE + 0x30)))
#define CLOCK_REF_DIV           (*((volatile uint32_t *)(CLOCKS_BASE + 0x34)))
#define CLOCK_REF_SELECTED      (*((volatile uint32_t *)(CLOCKS_BASE + 0x38)))
#define CLOCK_SYS_CTRL          (*((volatile uint32_t *)(CLOCKS_BASE + 0x3C)))
#define CLOCK_SYS_SELECTED      (*((volatile uint32_t *)(CLOCKS_BASE + 0x44)))
#define CLOCK_ADC_CTRL          (*((volatile uint32_t *)(CLOCKS_BASE + 0x6C)))
#define CLOCK_CLK_USB_CTRL      (*((volatile uint32_t *)(CLOCKS_BASE + 0x60)))

#define CLOCK_REF_SRC_XOSC      0x02
#define CLOCK_REF_SRC_SEL_MASK  0b1111
#define CLOCK_REF_SRC_SEL_XOSC  (1 << 2)

#define CLOCK_SYS_SRC_AUX           (1 << 0)
#define CLOCK_SYS_AUXSRC_PLL_SYS    (0x0 << 5)

#define CLOCK_ADC_ENABLE    (1 << 11)
#define CLOCK_ADC_ENABLED   (1 << 28)

#define CLOCK_USB_CTRL_ENABLE     (1 << 11)
#define CLOCK_USB_CTRL_AUXSRC_PLL_USB (0x0 << 5)

//--------------------------------------------------------------------+
// XOSC peripheral
//--------------------------------------------------------------------+
#define XOSC_BASE           0x40048000  
// Crystal Oscillator Registers
#define XOSC_CTRL       (*((volatile uint32_t *)(XOSC_BASE + 0x00)))
#define XOSC_STATUS     (*((volatile uint32_t *)(XOSC_BASE + 0x04)))
#define XOSC_DORMANT    (*((volatile uint32_t *)(XOSC_BASE + 0x08)))
#define XOSC_STARTUP    (*((volatile uint32_t *)(XOSC_BASE + 0x0C)))
#define XOSC_COUNT      (*((volatile uint32_t *)(XOSC_BASE + 0x10)))

// XOSC Values - See datasheet S8.2
#define XOSC_STARTUP_DELAY_1MS  47
#define XOSC_ENABLE         (0xfab << 12)
#define XOSC_RANGE_1_15MHz  0xaa0
#define XOSC_STATUS_STABLE  (1 << 31)

//--------------------------------------------------------------------+
// USB PLL Peripheral
//--------------------------------------------------------------------+
#define PLL_USB_BASE        0x40058000
#define PLL_USB_CS          (*((volatile uint32_t *)(PLL_USB_BASE + 0x00)))
#define PLL_USB_PWR         (*((volatile uint32_t *)(PLL_USB_BASE + 0x04)))
#define PLL_USB_FBDIV_INT   (*((volatile uint32_t *)(PLL_USB_BASE + 0x08)))
#define PLL_USB_PRIM        (*((volatile uint32_t *)(PLL_USB_BASE + 0x0C)))

// PLL Power bits  
#define PLL_PWR_PD          (1 << 0)    // Power down
#define PLL_PWR_DSMPD       (1 << 2)    // DSM power down  
#define PLL_PWR_POSTDIVPD   (1 << 3)    // Post divider power down
#define PLL_PWR_VCOPD       (1 << 5)    // VCO power down

// PLL Post divider bits
#define PLL_PRIM_POSTDIV1(X)    (((X) & PLL_PRIM_POSTDIV_MASK) << 16)
#define PLL_PRIM_POSTDIV2(X)    (((X) & PLL_PRIM_POSTDIV_MASK) << 12)
#define PLL_PRIM_POSTDIV_MASK   0x7

// PLL Control/Status bits
#define PLL_CS_LOCK         (1 << 31)
#define PLL_CS_BYPASS       (1 << 8)
#define PLL_CS_REFDIV_MASK  0b111111
#define PLL_CS_REFDIV(X)    (((X) & PLL_CS_REFDIV_MASK) << PLL_CS_REFDIV_SHIFT)
#define PLL_CS_REFDIV_SHIFT 0

//--------------------------------------------------------------------+
// Resets peripheral
//--------------------------------------------------------------------+
#define RESETS_BASE                     0x40020000u
#define RESETS_RESET_OFFSET             0x00u
#define RESETS_RESET_DONE_OFFSET        0x08u
#define RESETS_RESET_PLL_USB_BITS       (1u << 15)
#define RESETS_RESET_TIMER0_BITS        (1u << 23)
#define RESETS_RESET_USBCTRL_BITS       (1u << 28)

static inline void reset_block(uint32_t bits) {
    io_rw_32 *reset_set = (io_rw_32 *)hw_set_alias_untyped((void *)(RESETS_BASE + RESETS_RESET_OFFSET));
    *reset_set = bits;
}

static inline void unreset_block_wait(uint32_t bits) {
    io_rw_32 *reset_clr  = (io_rw_32 *)hw_clear_alias_untyped((void *)(RESETS_BASE + RESETS_RESET_OFFSET));
    io_ro_32 *reset_done = (io_ro_32 *)(RESETS_BASE + RESETS_RESET_DONE_OFFSET);
    *reset_clr = bits;
    while ((*reset_done & bits) != bits) { /* spin */ }
}

//--------------------------------------------------------------------+
// TIMER0 peripheral
//--------------------------------------------------------------------+
#define TIMER0_BASE         0x400b0000

// TIMER0 Registers
#define TIMER0_TIMELR       (*((volatile uint32_t *)(TIMER0_BASE + 0x0C)))
#define TIMER0_ALARM0       (*((volatile uint32_t *)(TIMER0_BASE + 0x10)))
#define TIMER0_INTE         (*((volatile uint32_t *)(TIMER0_BASE + 0x40)))
#define TIMER0_INTR         (*((volatile uint32_t *)(TIMER0_BASE + 0x3C)))

//--------------------------------------------------------------------+
// TICKS peripheral
//--------------------------------------------------------------------+
#define TICKS_BASE          0x40108000

// TICKS Registers
#define TICKS_TIMER0_CTRL   (*((volatile uint32_t *)(TICKS_BASE + 0x18)))
#define TICKS_TIMER0_CYCLES (*((volatile uint32_t *)(TICKS_BASE + 0x1C)))

//--------------------------------------------------------------------+
// NVIC (Nested Vectored Interrupt Controller)
//--------------------------------------------------------------------+
#define SCB_BASE            0xE000ED00

// SCB Registers
#define SCB_VTOR            (*((volatile uint32_t *)(SCB_BASE + 0x08)))
#define SCB_CPACR           (*((volatile uint32_t *)(SCB_BASE + 0x88)))

//--------------------------------------------------------------------+
// PBB (Peripheral Bridge B) - contains NVIC ISER/ICER registers
//--------------------------------------------------------------------+
#define PBB_BASE            0xE0000000

// PPB Registers
#define NVIC_ISER0          (*((volatile uint32_t *)(PBB_BASE + 0x0E100)))
#define NVIC_ISER1          (*((volatile uint32_t *)(PBB_BASE + 0x0E104)))
#define NVIC_ICER0          (*((volatile uint32_t *)(PBB_BASE + 0x0E180)))
#define NVIC_ICER1          (*((volatile uint32_t *)(PBB_BASE + 0x0E184)))

//--------------------------------------------------------------------+
// IRQ
//--------------------------------------------------------------------+
#define TIMER0_IRQ_0        0u
#define TIMER0_IRQ_1        1u
#define TIMER0_IRQ_2        2u
#define TIMER0_IRQ_3        3u
#define TIMER1_IRQ_0        4u
#define TIMER1_IRQ_1        5u
#define TIMER1_IRQ_2        6u
#define TIMER1_IRQ_3        7u
#define PWM_IRQ_WRAP_0      8u
#define PWM_IRQ_WRAP_1      9u
#define DMA_IRQ_0           10u
#define DMA_IRQ_1           11u
#define DMA_IRQ_2           12u
#define DMA_IRQ_3           13u
#define USBCTRL_IRQ         14u
#define PIO0_IRQ_0          15u
#define PIO0_IRQ_1          16u
#define PIO1_IRQ_0          17u
#define PIO1_IRQ_1          18u
#define PIO2_IRQ_0          19u
#define PIO2_IRQ_1          20u
#define IO_IRQ_BANK0        21u
#define IO_IRQ_BANK0_NS     22u
#define IO_IRQ_QSPI         23u
#define IO_IRQ_QSPI_NS      24u
#define SIO_IRQ_FIFO        25u
#define SIO_IRQ_BELL        26u
#define SIO_IRQ_FIFO_NS     27u
#define SIO_IRQ_BELL_NS     28u
#define SIO_IRQ_MTIMECMP    29u
#define CLOCKS_IRQ          30u
#define SPIO0_IRQ           31u
#define SPIO1_IRQ           32u
#define UART0_IRQ           33u
#define UART1_IRQ           34u
#define ADC_IRQ_FIFO        35u
#define I2C0_IRQ            36u
#define I2C1_IRQ            37u
#define OTP_IRQ             38u
#define TRNG_IRQ            39u
#define PROC0_IRQ_CTI       40u
#define PROC1_IRQ_CTI       41u
#define PLL_SYS_IRQ         42u
#define PLL_USB_IRQ         43u
#define POWMAN_IRQ_POW      44u
#define POWMAN_IRQ_TIMER    45u
#define SPAREIRQ_IRQ_0      46u
#define SPAREIRQ_IRQ_1      47u
#define SPAREIRQ_IRQ_2      48u
#define SPAREIRQ_IRQ_3      49u
#define SPAREIRQ_IRQ_4      50u
#define SPAREIRQ_IRQ_5      51u
#define NUM_IRQS            52u

#define PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY 0xffu
typedef void (*irq_handler_t)(void);

// IRQ Function prototypes
void irq_add_shared_handler(uint num, irq_handler_t handler, uint8_t order);
void irq_remove_handler(uint num, irq_handler_t handler);
void irq_set_enabled(uint num, bool enabled);

#if !defined(PICO_SL_EXCLUDE_IRQ_HANDLING)
extern irq_handler_t _irq_handlers[NUM_IRQS];

__attribute__((always_inline)) inline void pico_sl_isr(uint8_t num) {
    if (_irq_handlers[num]) _irq_handlers[num]();
}

#if defined(PICO_SL_IMPLEMENTATION)
irq_handler_t _irq_handlers[NUM_IRQS];

void irq_add_shared_handler(uint num, irq_handler_t handler, uint8_t order) {
    (void)order;
    _irq_handlers[num] = handler;
}

void irq_remove_handler(uint num, irq_handler_t handler) {
    (void)handler;
    irq_set_enabled(num, false);
    _irq_handlers[num] = NULL;
}

void irq_set_enabled(uint num, bool enabled) {
    if (enabled) {
        if (num < 32) {
            NVIC_ISER0 = (1u << num);
        } else {
            NVIC_ISER1 = (1u << (num - 32));
        }
    } else {
        if (num < 32) {
            NVIC_ICER0 = (1u << num);
        } else {
            NVIC_ICER1 = (1u << (num - 32));
        }
    }
}
#endif // PICO_SL_IMPLEMENTATION
#endif // PICO_SL_EXCLUDE_IRQ_HANDLING

//--------------------------------------------------------------------+
// Timer
// TIMELR latches TIMEHR on read; for time_us_32 we only need the
// lower 32 bits.
//--------------------------------------------------------------------+
#define TIMER_BASE      0x400B0000u
#define TIMER_TIMELR    (*(io_ro_32 *)(TIMER_BASE + 0x0Cu))

static inline uint32_t time_us_32(void) {
    return TIMER_TIMELR;
}

// RP2350 boot block structure
typedef struct {
    uint32_t start_marker;          // 0xffffded3, start marker
    uint8_t  image_type_tag;        // 0x42, image type
    uint8_t  image_type_len;        // 0x1, item is one word in size
    uint16_t image_type_data;       // 0b0001000000100001, RP2350, ARM, Secure, EXE
    uint8_t  type;                  // 0xff, size type, last item
    uint16_t size;                  // 0x0001, size
    uint8_t  pad;                   // 0
    uint32_t next_block;            // 0, link to self, no next block
    uint32_t end_marker;            // 0xab123579, end marker
} __attribute__((packed)) rp2350_boot_block_t;

#if defined(PICO_SL_IMPLEMENTATION)
// New lib stubs
#include <sys/stat.h>
void _exit(int status) { (void)status; while(1); }
int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; (void)st; return -1; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int offset, int whence) { (void)fd; (void)offset; (void)whence; return -1; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int _write(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return -1; }
int _sbrk(int incr) { (void)incr; return -1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
int _getpid(void) { return 1; }
#endif // PICO_SL_IMPLEMENTATION

#endif // PICO_H