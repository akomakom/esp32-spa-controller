#include <sys/types.h>

#ifndef __UTILS_H__
#define __UTILS_H__

#include "pgmspace.h"
#include <stdexcept>
#include <Preferences.h>
#include <ArduinoJson.h>

// 8 channel relay ESP32-WROOM module from China:
const int RELAY_PIN_1 = 32;
const int RELAY_PIN_2 = 33;
const int RELAY_PIN_3 = 25;
const int RELAY_PIN_4 = 26;
const int RELAY_PIN_5 = 27;
const int RELAY_PIN_6 = 14;
const int RELAY_PIN_7 = 12;
const int RELAY_PIN_8 = 13;
const int LED_PIN = 23;

class SpaControl {
    const int DEFAULT_MIN = 0;
    const int DEFAULT_MAX = 1;
public:
    SpaControl(const char *name);
    SpaControl(const char *name, u_int8_t min, u_int8_t max);

    virtual void toggle();
    virtual void applyOutputs();

    const char *name;
    u_int8_t min = DEFAULT_MIN;
    u_int8_t max = DEFAULT_MAX;
    u_int8_t value = 0;

private:
    void init(const char *name, u_int8_t min, u_int8_t max);
protected:
    void incrementValue();
};

class SimpleSpaControl : public SpaControl {
public:
    SimpleSpaControl(const char *name, u_int8_t pin);
    SimpleSpaControl(const char *name, u_int8_t pin, u_int8_t min, u_int8_t max);

    virtual void toggle();
    virtual void applyOutputs();

    u_int8_t pin;
};

/**
 * A two-relay wiring setup where one relay turns on power and the other (SPDT) relay
 * selects one of two speeds, eg for a pool pump.  Low is 1,0 and high is 1,1
 *
 */
class TwoSpeedSpaControl : public SpaControl {
public:
    TwoSpeedSpaControl(const char *name, u_int8_t pin_power, u_int8_t pin_speed);
    virtual void toggle();
    virtual void applyOutputs();

private:
    SimpleSpaControl* power;
    SimpleSpaControl* speed;
};

class SpaStatus {
private:
    StaticJsonDocument<200> jsonStatus;
    JsonObject jsonStatusControls = jsonStatus.createNestedObject("controls");
    JsonObject jsonStatusMetrics = jsonStatus.createNestedObject("metrics");

public:
    SpaStatus();

//    SpaControl *pump = new SimpleSpaControl("pump", RELAY_PIN_1);
//    SpaControl *pump_fast = new SimpleSpaControl("pump_fast", RELAY_PIN_2);
    SpaControl *pump = new TwoSpeedSpaControl("pump", RELAY_PIN_1, RELAY_PIN_2);
    SpaControl *blower = new SimpleSpaControl("blower", RELAY_PIN_3);
    SpaControl *heater = new SimpleSpaControl("heater", RELAY_PIN_4);
    SpaControl *ozone = new SimpleSpaControl("ozone", RELAY_PIN_5);


    char statusString[100];

    void updateStatusString();

//    SpaControl *controls[3] = {blower, heater, ozone};
    SpaControl *controls[4] = {pump, blower, heater, ozone};
//    SpaControl *controls[5] = {pump, pump_fast, blower, heater, ozone};

    SpaControl *findByName(const char *name);

    void applyControls();

};


#endif