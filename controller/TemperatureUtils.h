//
// Created by akom on 7/14/23.
//

#ifndef HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H
#define HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H

#include <TimeLib.h>

// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
// TODO: unhardcode:
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 9
//seconds:
#define TEMPERATURE_REQUEST_FREQUENCY 10

class TemperatureUtils {
private:

    time_t temperatureRequestedTime = 0;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    OneWire *oneWire;

// Pass our oneWire reference to Dallas Temperature.
    DallasTemperature *sensors;

public:
    void setup();
    void loop();

};


#endif //HOT_TUB_CONTROLLER_TEMPERATUREUTILS_H
