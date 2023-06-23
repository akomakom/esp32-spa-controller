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
* Reading temperature sensor data and tying it to controls (eg heater)
* Sheduling on/off behavior
* Overriding said schedule briefly (eg pump to high for 20 minutes)
* Overriding said schedule in the future (eg heater up 10 degrees for the weekend)

### Hardware design

* Simple controls use single relays
* 2-speed controls use two SPDT relays (1 for on, 2 for speed), wired in series.