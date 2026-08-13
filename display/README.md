ESP32 Spa Controller Touchscreen wireless remote (ESP32)
----

This component is intended for a touchscreen ESP32 dev kit. 
It communicates with the controller via ESP-NOW protocol without 
    using WiFi or any configuration (auto-discovery).
    The UI is dynamically constructed from data sent from the controller.


### Hardware
Default configuration is set up for a `Sunton ESP32-4827S043-R` 
This is a 488x272 resistive touchscreen.  
It's not a great module but it's cheap and works.  Touchscreen calibration is wonky.
To configure for different hardware, edit `gfx.h` and `touch.h`
![display-casing.png](readme%2Fdisplay-casing.png)
![display-on.png](readme%2Fdisplay-on.png)
### Software

This is a [PlatformIO](https://platformio.org/) project. All dependencies are declared in
`platformio.ini` and fetched from the registry on the first build — nothing needs to be
installed by hand.

#### Dependencies

Fetched automatically from the PlatformIO registry (see `lib_deps` in `platformio.ini`):

* `lvgl` — pinned to **8.3.11**
* `Arduino_GFX` (moononournation) — **>= 1.6.7** (exposes the RGB-panel bounce-buffer size as a
  constructor argument and handles PSRAM cache writeback, so no local patch is needed)
* `Time` (paulstoffregen)

The one **vendored** library is `XPT2046_Touchscreen`, under `lib/` (PlatformIO auto-includes
the project `lib/` directory). It is kept in-tree because it carries a one-line local patch —
the SPI clock is lowered to 500 kHz so the resistive-touch ADC settles and reads linearly on
this panel. Because `lib/` lives inside the source dir, `platformio.ini` uses
`build_src_filter = +<*> -<lib/>` so it is compiled once (as a library) rather than twice.

#### LVGL config

`lv_conf.h` is committed in this directory and found at build time via
`-I${PROJECT_DIR} -DLV_CONF_INCLUDE_SIMPLE` in `platformio.ini`, so there is no setup step.
If you ever change the pinned `lvgl` version, regenerate it from that version's
`lv_conf_template.h` (LVGL's options drift between releases) and keep the project-specific
settings, notably:

```c++
#define LV_COLOR_DEPTH        16
#define LV_TICK_CUSTOM        1
#define LV_FONT_MONTSERRAT_24 1   // card values / dialog text
#define LV_FONT_MONTSERRAT_40 1   // large setpoint number
```

Note: `LV_SPRINTF_USE_FLOAT` is left **0**, so `%f` must not be passed to
`lv_label_set_text_fmt` — the code formats temperatures with `snprintf`/integers instead.

#### Building

```shell
cd display
pio run -t upload        # compile + flash
pio device monitor       # serial console
pio run -t upload --upload-port /dev/ttyUSB1   # override the port from platformio.ini
```

The legacy `arduino-cli` `Makefile` is still present and works, but PlatformIO is the maintained
path (it pins the versions above and needs no global `~/Arduino/libraries` setup).