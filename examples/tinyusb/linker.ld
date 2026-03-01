/* Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
 *
 * MIT License
 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 2048K
    RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 520K
}

SECTIONS
{
    . = ORIGIN(FLASH);

    .vector_table : {
        KEEP(*(.vector_table*))
        . = ALIGN(4);
    }

    .boot_block : {
        KEEP(*(.boot_block*))
        . = ALIGN(4);
    }

    .text : {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        __text_end = .;
    }

    _sidata = LOADADDR(.data);
    .data ORIGIN(RAM) : AT(__text_end) {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    }
    __data_load = LOADADDR(.data);

    .bss (NOLOAD) : {
        _sbss = .;
        *(.bss*)
        . = ALIGN(4);
        _ebss = .;
    }

    /DISCARD/ : {
        *(.ARM.exidx*)
        *(.ARM.extab*)
    }
}

_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
_estack = _ram_end;

