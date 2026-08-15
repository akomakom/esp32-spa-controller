# Convenience wrapper around PlatformIO (pio) for the DISPLAY board.
#
# The real build configuration lives in platformio.ini. These targets are just
# shortcuts for common pio invocations.
#
# Shared headers (hot_tub_types.h, secrets.h) are pulled from the repo root via
# `build_flags = -I..` in platformio.ini, so there is no symlink step anymore.

PIO ?= pio
ENV ?= display
PORT ?= /dev/ttyUSB1

# Compile the firmware
compile:
	$(PIO) run -e $(ENV)

# Flash the firmware over USB
upload:
	$(PIO) run -e $(ENV) -t upload --upload-port $(PORT)

# Serial console of the board
monitor:
	$(PIO) device monitor --port $(PORT)

.PHONY: compile upload monitor
