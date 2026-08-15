ESP32 Custom Spa Controller
---- 

Control a custom hot tub using a bunch of relays and sensors.  

This is work-in-progress.

### Design Goals

* Control (via relays) of (at least):
  * 2-speed pumps
  * blower
  * heater
  * lights
  * (TBD) Valve actuators with smart power-off
* Reading temperature sensor data and tying it to controls (eg heater)
* Sheduling on/off behavior
* Overriding said schedule briefly (eg pump to high for 20 minutes)
* (TBD) Overriding said schedule in the future (eg heater up 10 degrees for the weekend)
* Web UI 
* Touchscreen UI using wireless display/ESP32 boards via ESP-NOW protocol
* OTA updates

### Hardware design

* Simple controls use single relays
* 2-speed controls use two SPDT relays (1 for on, 2 for speed), wired in series.

### Repository layout

The project is two independent boards, each its own [PlatformIO](https://platformio.org/) project:

* `controller/` — the ESP32 that drives the relays and sensors and serves the web UI.
* `display/` — an optional Sunton "CYD" touchscreen remote (ESP32-S3) that talks to the
  controller over ESP-NOW.

Shared message/type definitions live in `hot_tub_types.h` at the repo root and are pulled
into both projects via the include path (`-I..` in each `platformio.ini`), so there are no
header copies or symlinks to maintain.

### Build system

Both boards build with [PlatformIO](https://platformio.org/) (`pio`). All dependencies are
declared in each project's `platformio.ini` and fetched from the PlatformIO registry at build
time — nothing large is vendored in git. The one exception is `XPT2046_Touchscreen`, kept under
`display/lib/` because it carries a small local patch (see below).

Each project has a `Makefile` too, but it is only a thin convenience wrapper around `pio`
(`make upload`, `make monitor`, and — on the controller — a few HTTP helpers for OTA and live
SPIFFS sync). There is no separate arduino-cli build path anymore.

### Installation on a new ESP32

1. Copy `secrets.h.sample` to `secrets.h` and edit for your settings.

#### Controller

```shell
cd controller
pio run -t upload      # build + flash the firmware
pio run -t uploadfs    # upload the web UI (data/) to SPIFFS — once, and after UI changes
pio device monitor     # serial console
```

The web UI is a single self-contained `data/index.html` (no CDN/jQuery), so it works offline on
the controller's own AP. Firmware/filesystem/WiFi endpoints are behind HTTP Basic auth. See
[controller/README.md](controller%2FREADME.md) for details, credentials, and the `make`
convenience targets.

#### Display (optional touchscreen remote)

```shell
cd display
pio run -t upload
pio device monitor
```

`lvgl` and `Arduino_GFX` come from the registry via `platformio.ini`; only `XPT2046_Touchscreen`
is vendored (under `display/lib/`, auto-included by PlatformIO). The LVGL build config
(`display/lv_conf.h`) is committed, so no setup step is needed — but if you change the pinned
`lvgl` version, regenerate it from that version's `lv_conf_template.h`. Pin/panel/touch
configuration lives in `gfx.h` and `touch.h`. See [display/README.md](display%2FREADME.md).
