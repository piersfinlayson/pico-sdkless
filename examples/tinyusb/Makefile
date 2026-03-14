# Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
#
# MIT License

# Makefile for building the pico-sdkless example
#
# Automaticaly clones tinyusb
#
# Usage:
#   make all
#   make clean
#   make distclean

# If the arm-none-eabi toolchain is not in the PATH, set TOOLCHAIN to the
# directory containing it
TOOLCHAIN ?=
CC      := $(if $(TOOLCHAIN),$(TOOLCHAIN)/arm-none-eabi-gcc,arm-none-eabi-gcc)
OBJCOPY := $(if $(TOOLCHAIN),$(TOOLCHAIN)/arm-none-eabi-objcopy,arm-none-eabi-objcopy)
OBJDUMP := $(if $(TOOLCHAIN),$(TOOLCHAIN)/arm-none-eabi-objdump,arm-none-eabi-objdump)

# Bring in tinyusb
TINYUSB_DIR   := tinyusb-repo
$(shell [ -d $(TINYUSB_DIR) ] || git clone https://github.com/piersfinlayson/tinyusb.git $(TINYUSB_DIR))
include tinyusb.mk
TINYUSB_SRC_C := $(patsubst %,$(TINYUSB_DIR)/%,$(TINYUSB_SRC_C))
TINYUSB_INCLUDE_DIRS := $(patsubst %,$(TINYUSB_DIR)/%,$(TINYUSB_INCLUDE_DIRS))

# RP2350 ARM (non-secure) family ID
RP2350_FAMILY_ID := 0xe48bff59
FLASH_BASE       := 0x10000000

BUILD_DIR := build
BIN_PREFIX := pico-sl-tinyusb
ELF := $(BUILD_DIR)/$(BIN_PREFIX).elf
BIN := $(BUILD_DIR)/$(BIN_PREFIX).bin

TOOLSDIR := tools
UF2CONV := $(TOOLSDIR)/uf2conv.py
UF2     := $(BUILD_DIR)/$(BIN_PREFIX).uf2

EXAMPLE_SRC := src/main.c src/usb_descriptors.c

COMMON_DIR := ../common
include $(COMMON_DIR)/common.mk
COMMON_SRC := $(patsubst %,$(COMMON_DIR)/%,$(COMMON_SRC))
COMMON_LD := $(patsubst %,$(COMMON_DIR)/%,$(COMMON_LD))

CFLAGS := -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 \
          -O2 -Wall -Wextra -Werror \
          -ffunction-sections -fdata-sections \
          -MMD -MP -Iinclude -I../../include \
          -fno-builtin -nostdlib \
          $(addprefix -I,$(TINYUSB_INCLUDE_DIRS))

LDFLAGS := -nostdlib \
           -T $(COMMON_LD) \
           -Wl,--fatal-warnings \
		   -Wl,--no-warn-rwx-segments \
           -Wl,-Map=$(BUILD_DIR)/$(BIN_PREFIX).map

EXAMPLE_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(EXAMPLE_SRC))
COMMON_OBJ := $(patsubst $(COMMON_DIR)/%.c,$(BUILD_DIR)/common/%.o,$(COMMON_SRC))
TINYUSB_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TINYUSB_SRC_C))
ALL_OBJ     := $(EXAMPLE_OBJ) $(COMMON_OBJ) $(TINYUSB_OBJ)

.PHONY: all clean distclean

all: $(UF2)

$(UF2CONV):
	@mkdir -p $(TOOLSDIR)
	@echo "Downloading uf2conv.py..."
	@curl -sL https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2conv.py -o $@
	@curl -sL https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2families.json -o tools/uf2families.json
	@chmod +x $@

$(UF2): $(BIN) $(UF2CONV)
	@echo "Converting binary to UF2..."
	@python3 $(UF2CONV) --base $(FLASH_BASE) --family $(RP2350_FAMILY_ID) --output $@ $<

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(ALL_OBJ) ../../include/pico.h $(COMMON_LD)
	@echo "Linking..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $(ALL_OBJ) -o $@

$(BIN): $(ELF)
	@echo "Converting ELF to binary..."
	@$(OBJCOPY) -O binary $< $@
	@$(OBJDUMP) -s -j .vector_table $(ELF) > $(BUILD_DIR)/$(BIN_PREFIX).dis
	@$(OBJDUMP) -s -j .boot_block $(ELF) >> $(BUILD_DIR)/$(BIN_PREFIX).dis
	@$(OBJDUMP) -d -S $(ELF) >> $(BUILD_DIR)/$(BIN_PREFIX).dis

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

toolsclean:
	@echo "Cleaning downloaded tools..."
	@rm -rf $(TOOLSDIR)

distclean: clean
	@echo "Removing cloned repositories..."
	@rm -rf $(TINYUSB_DIR)

-include $(ALL_OBJ:.o=.d)