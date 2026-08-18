//
// Created by akom on 7/14/23.
//

#ifndef HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H
#define HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H

#include <vector>
#include <sys/types.h>
#include <TimeLib.h>

// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

#include "core.h"

// Data wire is plugged into port 4 on the Arduino
// TODO: unhardcode:
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 9
//seconds:
#define TEMPERATURE_REQUEST_FREQUENCY 10

// How long a cached reading remains trustworthy. If a sensor stops responding
// (bad wire, dead probe) we must NOT keep heating against a stale value forever,
// so past this age the reading is considered invalid: getTempF()/getTempC()
// report 0 and the heater is locked out. Kept well above
// TEMPERATURE_REQUEST_FREQUENCY so a couple of missed reads don't trip it.
//seconds:
#define MAX_TEMP_VALIDITY_SECONDS 60

// sanity checks (native unit is Fahrenheit)
#define MINIMUM_VALID_TEMP_F 1
#define MAXIMUM_VALID_TEMP_F 120

class SensorAddressMapping {
public:
    u_int8_t id;
    DeviceAddress addr;
};


class TemperatureUtils {

private:
    const char* KEY_TEMP_WATER_ADDRESS = "1w_addr_water";
    std::vector<SensorAddressMapping*> map;
    std::vector<float> temperatureCache;
    // Wall-clock time each cache entry last received a valid reading; 0 = never.
    // Used by isValid() to expire stale readings (see MAX_TEMP_VALIDITY_SECONDS).
    std::vector<time_t> temperatureUpdatedTime;

    time_t temperatureRequestedTime = 0;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    OneWire *oneWire;

// Pass our oneWire reference to Dallas Temperature.
    DallasTemperature *sensors;

    void readTemperatures();
    void printAddress(DeviceAddress deviceAddress);

public:
    void setup();
    void loop();
    // Native unit is Fahrenheit. getTempC() converts for display convenience.
    // Both return 0 when the reading is stale/invalid (see isValid()).
    float getTempC(u_int8_t sensorIndex);
    float getTempF(u_int8_t sensorIndex);
    float getTempFByID(u_int8_t id);

    // True only if this sensor produced a valid reading within
    // MAX_TEMP_VALIDITY_SECONDS. A failed/absent sensor reads false, which is
    // the heater's cue to lock out rather than heat against a stale value.
    bool isValid(u_int8_t sensorIndex);

    // Unit conversion helpers
    static float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }
    static float fToC(float f) { return (f - 32.0f) * 5.0f / 9.0f; }
};


#endif //HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H
