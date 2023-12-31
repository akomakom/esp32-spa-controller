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

### Installation on a new ESP32

1. Copy secrets.h.sample to secrets.h and edit for your settings

#### Controller

In `controller/` subdirectory:

2. Build and upload the controller code using either:
  * Arduino IDE (ensure you have ESP32 board support installed)
  * arduino-cli, if installed (try `make upload`)
3. Reboot the board and check that it's on the network and listening on port 80
4. Upload the web interface using either:
  * Arduino IDE: Use [this plugin](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)
  * Command Line: try `make upload-spiffs-via-http`

See [README.md](controller%2FREADME.md) in that directory

#### Display

Instructions are similar to Controller
See [README.md](display%2FREADME.md) in that directory
