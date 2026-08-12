
### Hardware 

Any ESP32 with enough pins to drive relays should work.  A pre-integrated dev board with relays can save some time.
![controller-installed.png](readme%2Fcontroller-installed.png)
![spa-controller-schematic.png](readme%2Fspa-controller-schematic.png)
### Software Installation 
#### Libraries

* Arduino_JSON (Benoit Blanchon)
* OneWire
* DallasTemperature

#### Binary

Two supported build paths:

* **arduino-cli** (original): `make upload monitor`
* **PlatformIO** (recommended, pins library versions and needs no header symlinks):
  `cd controller && pio run -t upload && pio device monitor`

#### Web UI

The UI is a single self-contained `data/index.html` — no jQuery/CDN dependencies,
so it works fully offline on the device's own AP. It reads the C/F unit from the
controller, so the toggle in the on-screen Settings dialog changes both the web and
the touchscreen display.

One-time step: Use Arduino IDE to upload SPIFFS to initialize the partition:
`Tools->ESP32 Sketch Data Upload`  (You'll need the ESP32 additional boards installed)

If you do not see that menu item, install https://github.com/me-no-dev/arduino-esp32fs-plugin

With PlatformIO: `pio run -t uploadfs` uploads the `data/` directory.

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