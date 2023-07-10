# This Makefile contains macros for using command-line, eg
# arduino-cli instead of arduino IDE

ARDUINO_CLI ?= /opt/arduino-cli/arduino-cli
PORT ?= /dev/ttyUSB2
FQBN ?= esp32:esp32:esp32s3
FQBN_PATH = $(shell echo ${FQBN} | tr : .)
#MKSPIFFS ?= /opt/arduino-cli/mkspiffs
#SPIFFS_OUTPUT_FILE=build/spiffs.bin


compile:
	${ARDUINO_CLI} compile --fqbn ${FQBN} .

# TBD
#spiffs:
#	${MKSPIFFS} -c data ${SPIFFS_OUTPUT_FILE}


# Upload the firmware
upload: compile
	 ${ARDUINO_CLI} upload -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s . -v

# Serial console of the board
monitor:
	while [ 1 ] ; do ${ARDUINO_CLI} monitor -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s -c baudrate=115200 . ; done

# Check that the named variable is set
guard-%:
	@ if [ "${${*}}" = "" ]; then \
		echo "Environment variable $* not set"; \
		exit 1; \
	fi
