# This Makefile contains macros for using command-line, eg
# arduino-cli instead of arduino IDE

ARDUINO_CLI ?= /opt/arduino-cli/arduino-cli
PORT ?= /dev/ttyUSB0
FQBN ?= esp32:esp32:esp32
FQBN_PATH = $(shell echo ${FQBN} | tr : .)
#MKSPIFFS ?= /opt/arduino-cli/mkspiffs
#SPIFFS_OUTPUT_FILE=build/spiffs.bin


compile:
	${ARDUINO_CLI} compile --fqbn esp32:esp32:esp32 .

# TBD
#spiffs:
#	${MKSPIFFS} -c data ${SPIFFS_OUTPUT_FILE}


# Upload the firmware
upload: compile
	 ${ARDUINO_CLI} upload -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s . -v

# Serial console of the board
monitor:
	${ARDUINO_CLI} monitor -p ${PORT} --fqbn ${FQBN} --discovery-timeout 30s -c baudrate=115200 .

# Once firmware is uploaded, it is possible to use this to sync contents of data/ to the ESP
# without using mkspiffs or arduino IDE's spiffs plugin (this does not handle delete)
# Note that initial upload must be done first (eg via Arduino IDE or mkspiffs)
upload-spiffs-via-http: guard-TARGET_IP
	for file in data/*.* ; do echo Uploading $$file ; curl -X POST -F "data=@$$file"  http://${TARGET_IP}/edit ; done

upload-spiffs-via-http-inotifywait: guard-TARGET_IP
	#cd data && inotifywait -m . -e close_write -r -m | grep -v '~' | while read dir action file; do echo $$file ; curl -X POST -F "data=@$$file"  http://${TARGET_IP}/edit ; done
	cd data && inotifywait -m . -e close_write --exclude '.*~' | while read DIR ACTION FILE ; do echo Uploading $$FILE ; curl -X POST -F "data=@$$FILE"  http://${TARGET_IP}/edit ; done

# Check that the named variable is set
guard-%:
	@ if [ "${${*}}" = "" ]; then \
		echo "Environment variable $* not set"; \
		exit 1; \
	fi
