// Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
//
// MIT License

// Reset vectors and handling for the pico-sdkless examples

#include "pico.h"

// Forward declarations
extern void example_main(void);
void reset(void);
void isr_timer0_irq_0(void);
void isr_usbctrl_irq(void);

// Default handlers
void default_handler(void)      { __asm volatile ("bkpt #0"); }
void default_isr_handler(void)  { __asm volatile ("bkpt #1"); }
void nmi_handler(void)          { __asm volatile ("bkpt #2"); }
void hardfault_handler(void)    { __asm volatile ("bkpt #3"); }
void memmanage_handler(void)    { __asm volatile ("bkpt #4"); }
void busfault_handler(void)     { __asm volatile ("bkpt #5"); }
void usagefault_handler(void)   { __asm volatile ("bkpt #6"); }

// Declare stack section
extern uint32_t _estack;

// Vector table - must be placed at the start of flash
__attribute__ ((section(".vector_table"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack,      // Initial stack pointer
    reset,                         // Reset handler
    nmi_handler,                   // NMI handler
    hardfault_handler,             // Hard fault handler
    memmanage_handler,             // MPU fault handler
    busfault_handler,              // Bus fault handler
    usagefault_handler,            // Usage fault handler
    0, 0, 0, 0,                    // Reserved
    default_handler,               // SVCall handler
    default_handler,               // Debug monitor handler
    0,                             // Reserved
    default_handler,               // PendSV handler
    default_handler,               // SysTick handler

    // Peripheral interrupt handlers follow.  See datasheet 3.2 for locations 
    // 0-3
    isr_timer0_irq_0, default_isr_handler, default_isr_handler, default_isr_handler,
    // 4-7
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 8-11
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 12-15 - 14 = USBCTRL_IRQ
    default_isr_handler, default_isr_handler, isr_usbctrl_irq, default_isr_handler,
    // 16-19
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 20-23
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 24-27
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 28-31
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 32-35
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 36-39
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 40-43
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 44-47
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
    // 48-51
    default_isr_handler, default_isr_handler, default_isr_handler, default_isr_handler,
};

__attribute__((section(".boot_block")))
const rp2350_boot_block_t rp2350_arm_boot_block = {
    .start_marker    = 0xffffded3,
    .image_type_tag  = 0x42,
    .image_type_len  = 0x1,
    .image_type_data = 0b0001000000100001,
    .type            = 0xff,
    .size            = 0x0001,
    .pad             = 0,
    .next_block      = 0,
    .end_marker      = 0xab123579
};



// TIMER0_IRQ_0 handler - call into the pico-sdkless handler
void isr_timer0_irq_0(void) {
    pico_sl_isr(TIMER0_IRQ_0);
}

// USBCTRL IRQ handler - call into the pico-sdkless handler
void isr_usbctrl_irq(void) {
    pico_sl_isr(USBCTRL_IRQ);
}

// Linker variables for the .data and .bss sections, defined in linker.ld
extern uint32_t __ramfunc_load; // Start of .ramfunc section in flash
extern uint32_t __ramfunc_start; // Start of .ramfunc section in RAM
extern uint32_t __ramfunc_end;   // End of .ramfunc section in RAM
extern uint32_t _sidata;    // Start of .data section in FLASH
extern uint32_t _sdata;     // Start of .data section in RAM
extern uint32_t _edata;     // End of .data section in RAM
extern uint32_t _sbss;      // Start of .bss section in RAM
extern uint32_t _ebss;      // End of .bss section in RAM

// Reset handler
void reset(void) {
    // Enable hard floating point support on both cores:
    SCB_CPACR |= (0xF << 20); // Enable CP10 and CP11 full access
    __asm volatile ("dsb");
    __asm volatile ("isb");

    // Copy ramfunc section from flash to RAM
    memcpy(&__ramfunc_start, &__ramfunc_load, (unsigned int)(&__ramfunc_end - &__ramfunc_start));

    // Copy data section from flash to RAM
    memcpy(&_sdata, &_sidata, (unsigned int)((char*)&_edata - (char*)&_sdata));
    
    // Zero out bss section  
    memset(&_sbss, 0, (unsigned int)((char*)&_ebss - (char*)&_sbss));
    
    // Call the main function
    example_main();
    
    // In case main returns
    while(1);
}
