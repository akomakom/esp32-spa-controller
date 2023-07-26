# This Makefile contains macros for using command-line, eg
# arduino-cli instead of arduino IDE

ARDUINO_CLI ?= /opt/arduino-cli/arduino-cli
BAUD ?= 115200
PORT ?= /dev/ttyUSB2
FQBN ?= esp32:esp32:esp32s3
BOARD_OPTIONS ?= PSRAM=opi,FlashSize=16M,DebugLevel=warn
FQBN_PATH = $(shell echo ${FQBN} | tr : .)
#MKSPIFFS ?= /opt/arduino-cli/mkspiffs
#SPIFFS_OUTPUT_FILE=build/spiffs.bin

# Because arduino can't handle a relative path parent directory include, we symlink these:
# On windows you may need to copy the files to each project instead
secrets.h:
	ln -s ../secrets.h .

hot_tub_types.h:
	ln -s ../hot_tub_types.h .

compile: secrets.h hot_tub_types.h
	${ARDUINO_CLI} compile --fqbn ${FQBN} --board-options "${BOARD_OPTIONS}" .

# TBD
#spiffs:
#	${MKSPIFFS} -c data ${SPIFFS_OUTPUT_FILE}


# Upload the firmware
upload: compile
	 ${ARDUINO_CLI} upload -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s . -v

# Serial console of the board
monitor:
	while [ 1 ] ; do ${ARDUINO_CLI} monitor -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s -c baudrate=${BAUD} . ; done

