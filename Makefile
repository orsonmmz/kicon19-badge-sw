# Include the common Makefile, which will also include the project specific
# config.mk file.
BUILD_DIR = build
MAKEFILE_PATH = sam/utils/make/Makefile.sam.in
include $(MAKEFILE_PATH)

# JTAG interface used by OpenOCD
JTAG_IFACE ?= jlink

commands_def.py: commands_def.h
	$(CC) -DGENERATE_PYTHON -E -P $< -o python/$@

flash: $(TARGET_FLASH)
	# GPNVM bit 1 has to be active in order to boot the flashed firmware
	# instead of the bootloader
	openocd -f interface/$(JTAG_IFACE).cfg -f scripts/at91sam4sx16x.cfg \
	    -c "init; reset halt; at91sam4 gpnvm set 1; program $(TARGET_FLASH) reset verify exit"

debug: $(TARGET_FLASH)
	# Starts openocd and gdb halted on the reset handler
	openocd -f interface/$(JTAG_IFACE).cfg -f scripts/at91sam4sx16x.cfg -c "init; reset halt" &
	$(GDB) -ex "target remote localhost:3333" $(TARGET_FLASH)
	killall openocd
