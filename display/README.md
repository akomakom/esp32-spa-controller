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

#### Libraries Required

These can be installed using Arduino IDE.

* ESP32 Boards (3.x)
* LVGL (8.4.0).  See setup steps below.
* Arduino_GFX_Library (version 1.5.0)
* XPT2046_Touchscreen (or your touchscreen's driver)
* Time https://github.com/PaulStoffregen/Time


#### LVGL Setup

LVGL needs to be configured at the system level as follows

**Note** you should repeat this step every time you upgrade `lvgl`, as the defaults vary between versions.

1. Copy `~/Arduino/libraries/lvgl/lv_conf_template.h` to `~/Arduino/libraries/lv_conf.h` (not a typo, copied to libraries dir)
2. Edit as follows:
3. Change "if 0" at the top to "if 1"
4. You may need to experiment with `LV_MEM_CUSTOM` (or not).  The following settings work for `LV_MEM_CUSTOM 0` if you need it.
   ```c++
   #define LV_MEM_SIZE (96U * 1024U)
   #define LV_MEM_POOL_INCLUDE <esp32-hal-psram.h>
   #define LV_MEM_POOL_ALLOC ps_malloc
   ```
5. Turn these on:
    ```c++
   #define LV_TICK_CUSTOM 1   (no idea why)
   #define LV_COLOR_16_SWAP 1 (possibly only useful for Squareline)
   #define LV_SPRINTF_USE_FLOAT 1  (for temperature display)
   #define LV_FONT_MONTSERRAT_20 1 (for larger fonts)
   #define LV_FONT_MONTSERRAT_40 1 (even larger fonts)
   ```

#### Building
Makefile is provided for building on *nix systems using command-line tools (eg `arduino-cli`).
See overridable defaults at the top of the `Makefile`.

Typical usage (once tooling is set up) is:
```shell
# compile, upload, then start serial monitor
make upload monitor
make upload monitor PORT=/dev/ttyUSB3 # change port.  
```
If building on windows, review Makefiles and perform steps manually.