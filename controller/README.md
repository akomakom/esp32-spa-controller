
### Hardware 

Any ESP32 with enough pins to drive relays should work.  A pre-integrated dev board with relays can save some time.
![controller-installed.png](readme%2Fcontroller-installed.png)
![spa-controller-schematic.png](readme%2Fspa-controller-schematic.png)
### Software Installation 
#### Libraries

With PlatformIO these are fetched automatically from the registry (see `lib_deps` in
`platformio.ini`); with the Arduino IDE, install them by hand:

* ArduinoJson (bblanchon)
* DallasTemperature (milesburton)
* OneWire (paulstoffregen)
* Time (paulstoffregen)

#### Binary

Build and flash with PlatformIO (pins library versions, needs no header symlinks):

```shell
cd controller
pio run -t upload      # build + flash
pio device monitor     # serial console
```

The `Makefile` in this directory wraps the same commands for convenience
(`make upload`, `make monitor`, `make uploadfs`) — see the HTTP helpers below.

Note: the controller board (`esp32dev`, `/dev/ttyUSB0` by default) does not auto-enter the
bootloader on some USB-serial adapters — hold BOOT / tap EN before `pio run -t upload` if the
upload fails to start.

#### Web UI

The UI is a single self-contained `data/index.html` — no jQuery/CDN dependencies,
so it works fully offline on the device's own AP. It reads the C/F unit from the
controller, so the toggle in the on-screen Settings dialog changes both the web and
the touchscreen display.

Upload it (and initialize the SPIFFS partition) with `pio run -t uploadfs`, or
`make uploadfs`, which flashes the `data/` directory to the device.

#### Security

Firmware update (`/update`), the filesystem editor (`/edit`, `/list`) and WiFi
configuration (`/configureWifi`) are protected by HTTP Basic auth. Set
`OTA_AUTH_USER` / `OTA_AUTH_PASS` in `secrets.h` (defaults to `admin`/`admin` if unset).

### Web Development for the UI

`make upload-spiffs-via-http-inotifywait`  
This will automatically upload any changed files in `data/` directory on save


### Configuration

* Set up WiFi SSID/Passwords in secrets.h (copy secrets.h.example).  These credentials are for the AP on the controller, not your home network.
* Connect to this network with your phone or laptop
* Browse to http://192.168.4.1
* Click the tiny gear button in the bottom-right corner
* The controller will reboot.  Disconnect from the temporary network. 