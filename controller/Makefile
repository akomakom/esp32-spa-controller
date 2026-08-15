# Convenience wrapper around PlatformIO (pio) for the CONTROLLER board.
#
# The real build configuration lives in platformio.ini. These targets are just
# shortcuts for common pio invocations plus a few HTTP-based helpers that pio
# does not provide (OTA firmware push, live SPIFFS sync of the web UI).
#
# Shared headers (hot_tub_types.h, secrets.h) are pulled from the repo root via
# `build_flags = -I..` in platformio.ini, so there is no symlink step anymore.

PIO ?= pio
ENV ?= controller
PORT ?= /dev/ttyUSB0
FIRMWARE = .pio/build/$(ENV)/firmware.bin

# HTTP Basic auth for the /update and /edit endpoints, parsed from the shared
# secrets.h (same #defines the firmware uses). Falls back to admin/admin, which
# is what the firmware defaults to when the #defines are absent. Override on the
# command line if needed, e.g. `make upload-via-http OTA_AUTH_USER=foo ...`.
SECRETS ?= ../secrets.h
parse-secret = $(shell sed -n 's/^[[:space:]]*\#define[[:space:]]\+$(1)[[:space:]]\+"\(.*\)".*/\1/p' $(SECRETS) 2>/dev/null)
OTA_AUTH_USER := $(or $(call parse-secret,OTA_AUTH_USER),admin)
OTA_AUTH_PASS := $(or $(call parse-secret,OTA_AUTH_PASS),admin)
CURL := curl -u '$(OTA_AUTH_USER):$(OTA_AUTH_PASS)'
# Compile the firmware
compile:
	$(PIO) run -e $(ENV)

# Flash the firmware over USB
upload:
	$(PIO) run -e $(ENV) -t upload --upload-port $(PORT)

# Build + upload the data/ web UI to SPIFFS
uploadfs:
	$(PIO) run -e $(ENV) -t uploadfs --upload-port $(PORT)

# Serial console of the board
monitor:
	$(PIO) device monitor --port $(PORT)

# POST binary firmware to the /update endpoint instead of flashing via USB.
# Fails the target unless the endpoint returns HTTP 200.
upload-via-http: compile guard-TARGET_IP
	@echo "Uploading $(FIRMWARE) to http://${TARGET_IP}/update as $(OTA_AUTH_USER)" ; \
	code=$$($(CURL) --progress-bar -o /dev/null -w '%{http_code}' \
		-u "$(OTA_AUTH_USER):$(OTA_AUTH_PASS)" \
		-F update=@$(FIRMWARE) http://${TARGET_IP}/update) ; \
	echo "HTTP $$code" ; \
	if [ "$$code" != "200" ]; then echo "Firmware upload failed (HTTP $$code)" >&2 ; exit 1 ; fi

# Once firmware is uploaded, sync the contents of data/ to the ESP over HTTP
# without a full SPIFFS reflash (this does not handle deletes).
# Note that an initial SPIFFS upload must be done first (make uploadfs).
upload-spiffs-via-http: guard-TARGET_IP
	for file in data/*.* ; do \
		echo -n "Uploading $$file ... " ; \
		$(CURL) -w '%{http_code}' -X POST -F "data=@$$file"  http://${TARGET_IP}/edit ; \
		echo "" ; \
	done

upload-spiffs-via-http-inotifywait: guard-TARGET_IP
	cd data && inotifywait -m . -e close_write --exclude '.*~' | while read DIR ACTION FILE ; do \
		echo -n "Uploading $$FILE ... " ; \
		$(CURL) -w '%{http_code}' -X POST -F "data=@$$FILE" http://${TARGET_IP}/edit ; \
		echo "" ; \
	done

# Check that the named variable is set
guard-%:
	@ if [ "${${*}}" = "" ]; then \
		echo "Environment variable $* not set"; \
		exit 1; \
	fi

.PHONY: compile upload uploadfs monitor upload-via-http upload-spiffs-via-http upload-spiffs-via-http-inotifywait
