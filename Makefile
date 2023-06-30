# This Makefile contains macros for using the arduino-cli instead of arduino IDE

ARDUINO_CLI ?= /opt/arduino-cli/arduino-cli
PORT ?= /dev/ttyUSB0
FQBN ?= esp32:esp32:esp32
#MKSPIFFS ?= /opt/arduino-cli/mkspiffs
#SPIFFS_OUTPUT_FILE=build/spiffs.bin


compile:
	 ${ARDUINO_CLI} compile --fqbn esp32:esp32:esp32 .


#spiffs:
#	${MKSPIFFS} -c data ${SPIFFS_OUTPUT_FILE}


upload: compile
	 ${ARDUINO_CLI} upload -p ${PORT} --fqbn ${FQBN}  --discovery-timeout 30s . -v

