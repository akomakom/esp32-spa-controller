
### Hardware 

Any ESP32 with enough pins to drive relays should work.  A pre-integrated dev board with relays can save some time.
![controller-installed.png](readme%2Fcontroller-installed.png)
![spa-controller-schematic.png](readme%2Fspa-controller-schematic.png)
### Software Installation 
#### Libraries

* Arduino_JSON
* OneWire
* DallasTemperature

#### Binary

`make upload monitor`

#### Web UI

One-time step: Use Arduino IDE to upload SPIFFS to initialize the partition:
`Tools->ESP32 Sketch Data Upload`  (You'll need the ESP32 additional boards installed)

### Web Development for the UI

`make upload-spiffs-via-http-inotifywait`  
This will automatically upload any changed files in `data/` directory on save
