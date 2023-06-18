#include <sys/types.h>
#ifndef __UTILS_H__
#define __UTILS_H__

#include "pgmspace.h"
#include <stdexcept>
#include <Preferences.h>
#include <ArduinoJson.h>

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
    SpaControl(const char* name);
    SpaControl(const char* name, u_int8_t min, u_int8_t max);
    void toggle();
    void applyOutputs();

    const char* name;
    u_int8_t min = DEFAULT_MIN;
    u_int8_t max = DEFAULT_MAX;
    u_int8_t value = 0;

  private:
    void init(const char* name, u_int8_t min, u_int8_t max);
};

class SimpleSpaControl : public SpaControl {
  public:
    SimpleSpaControl(const char* name, u_int8_t pin);
    SimpleSpaControl(const char* name, u_int8_t pin, u_int8_t min, u_int8_t max);
    void toggle();
    void applyOutputs();
    u_int8_t pin;
};

// class TwoSpeedSpaControl : public SpaControl {
//   public:
//     TwoSpeedSpaControl(const char* name, u_int8_t pin_power, u_int8_t pin_speed) : SpaControl(name);
// };

class SpaStatus {
  private:
    StaticJsonDocument<200> jsonStatus;
    JsonObject jsonStatusControls = jsonStatus.createNestedObject("controls");
    JsonObject jsonStatusMetrics = jsonStatus.createNestedObject("metrics");

  public:
    SpaStatus();
    SpaControl* pump      = new SimpleSpaControl("pump",      RELAY_PIN_1); 
    SpaControl* pump_fast = new SimpleSpaControl("pump_fast", RELAY_PIN_2); 
    SpaControl* blower    = new SimpleSpaControl("blower",    RELAY_PIN_3);
    SpaControl* heater    = new SimpleSpaControl("heater",    RELAY_PIN_4);
    SpaControl* ozone     = new SimpleSpaControl("ozone",     RELAY_PIN_5);


    char statusString[100];
    void updateStatusString();

    SpaControl* controls[5] = { pump, pump_fast, blower, heater, ozone };

    SpaControl* findByName(const char* name);

    void applyControls();
  
};


#endif