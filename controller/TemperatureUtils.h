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

// sanity checks
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
//    float getTempC(u_int8_t sensorIndex);
    float getTempF(u_int8_t sensorIndex);
    float getTempFByID(u_int8_t id);
};


#endif //HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H
