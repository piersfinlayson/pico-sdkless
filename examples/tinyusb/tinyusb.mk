# C source files
TINYUSB_SRC_C += \
	src/tusb.c \
	src/common/tusb_fifo.c \
	src/device/usbd.c \
	src/device/usbd_control.c \
	src/class/cdc/cdc_device.c \
	src/portable/raspberrypi/rp2040/dcd_rp2040.c \
	src/portable/raspberrypi/rp2040/rp2040_usb.c

TINYUSB_INCLUDE_DIRS += \
	src \
	src/common \
	src/device \
	src/class/cdc \
	src/portable/raspberrypi/rp2040
