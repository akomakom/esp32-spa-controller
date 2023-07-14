#include <sys/types.h>

#ifndef HOT_TUB_CONTROLLER_HOTTUBUTILS_H
#define HOT_TUB_CONTROLLER_HOTTUBUTILS_H

#include <vector>
#include "pgmspace.h"
#include <stdexcept>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

// NTP stuff
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "TemperatureUtils.h"

// needs to be large enough to allocate json document and string representation
#define STATUS_LENGTH 600

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

extern Preferences app_preferences;


class SpaSensor {

};


/**
 * A Scheduler that can be attached to any control
 * A control will consult the scheduler to see if the value should be schedule-based
 *
 * Scheduler supports two types of automation:
 * 1) Normal: On for a certain percentage of the day, every day.
 * 2) Override: Temporarily override the normal schedule (whether one is set or not), one time, for a set duration.
 *
 * For example, 2-speed pump scheduler:
 * - Pump on low for 50% of the day, in 5 segments = 10% on, 10% off, all day (2.4 hours on, 2.4 hours off):
 *      normalSchedule(50, 5, 1, 0);
 * - Override: someone pushed the Boost button (high speed): High Speed for 20 minutes, then back to above:
 *      scheduleOverride(now(), now() + 20 * SECS_PER_MIN, 2)
 *
 */
class SpaControlScheduler {
    const int DEFAULT_MIN = 0;
    const int DEFAULT_MAX = 1;
public:

    /**
     * Schedule on-time through the day.
     * @param percentageOfDayOnTime eg 50
     * @param numberOfTimesToRun eg 5, meaning run 50% in a 24 hour period, break up on-time into 5 segments
     * @param normalValueOn when in on phase, this should be the control's value
     * @param normalValueOff when in off phase, this should be the control's value
     */
    void normalSchedule(u_int8_t percentageOfDayOnTime, u_int8_t numberOfTimesToRun, u_int8_t normalValueOn, u_int8_t normalValueOff);

    /**
     *  Override normal schedule during this period
     * @param startTime
     * @param endTime
     * @param valueOverride
     */
    void scheduleOverride(time_t startTime, time_t endTime, u_int8_t valueOverride);

    void cancelOverride();
    /**
     *
     * @return true if override schedule (specifically) is in effect
     */
    bool isOverrideScheduleEnabled();

    /**
     *
     * @return remaining seconds or 0 if not in override mode
     */
    time_t getOverrideScheduleRemainingTime();

    /**
     * @return scheduled value if schedule is enabled
     * If override is enabled, returns overrideValue
     * Returns normalValueOn or normalValueOff otherwise
     */
    u_int8_t getScheduledValue();

    // min and max bounds for value, for arg checking
    u_int8_t min = DEFAULT_MIN;
    u_int8_t max = DEFAULT_MAX;

private:
    /**
     * Utility that will raise an exception for out of bounds
     * @param value
     */
    void checkBounds(u_int8_t value);

    static const u_int8_t SCHEDULER_DISABLED_VALUE = -1;
    // default is always off:
    u_int8_t percentageOfDayOnTime = 0;
    u_int8_t numberOfTimesToRun = 1; // stay on
    u_int8_t normalValueOn = 1;
    u_int8_t normalValueOff = 0;

    time_t overrideStartTime = now();
    time_t overrideEndTime = now();
    u_int8_t overrideValue = SCHEDULER_DISABLED_VALUE;

    // For normal schedule math:
    // These are normally set by calling normalSchedule():
    float onOffLengthPercentage = 100; //defaults based on values of variables above (always off)
    float onVsOff = 0; //defaults based on values of variables above (always off)
};

class SpaControl : public SpaControlScheduler {
public:
    SpaControl(const char *name, const char *type);

    virtual void toggle();

    /**
     *
     * @return either this->value or this->scheduler value if scheduler is in effect
     */
    virtual u_int8_t getEffectiveValue();

    virtual void applyOutputs();

    const char *name;
    const char *type;
    JsonObject jsonStatus;

private:
    void init(const char *name, u_int8_t min, u_int8_t max);

protected:
    u_int8_t getNextValue(); // helper for toggle()
};

class SimpleSpaControl : public SpaControl {
public:
    SimpleSpaControl(const char *name, u_int8_t pin);

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

    u_int8_t pinPower;
    u_int8_t pinSpeed;
};

class SpaStatus {
private:
    StaticJsonDocument<STATUS_LENGTH> jsonStatus;
    JsonArray jsonStatusControls = jsonStatus.createNestedArray("controls");
    JsonObject jsonStatusMetrics = jsonStatus.createNestedObject("metrics");
    TemperatureUtils temperatureUtils;
    WiFiUDP ntpUDP;
    NTPClient *timeClient;

public:
    SpaStatus();

//    SpaControl *pump = new SimpleSpaControl("pump", RELAY_PIN_1);
//    SpaControl *pump_fast = new SimpleSpaControl("pump_fast", RELAY_PIN_2);
    SpaControl *pump = new TwoSpeedSpaControl("pump", RELAY_PIN_1, RELAY_PIN_2);
    SpaControl *blower = new SimpleSpaControl("blower", RELAY_PIN_3);
    SpaControl *heater = new SimpleSpaControl("heater", RELAY_PIN_4);
    SpaControl *ozone = new SimpleSpaControl("ozone", RELAY_PIN_5);

    char statusString[STATUS_LENGTH];

    void updateStatusString();

    std::vector<SpaControl*> controls = {pump, blower, heater, ozone};
//    SpaControl *controls[3] = {blower, heater, ozone};
//    SpaControl *controls[4] = {pump, blower, heater, ozone};

//    SpaControl *controls[5] = {pump, pump_fast, blower, heater, ozone};

    SpaControl *findByName(const char *name);

    void setup();
    void loop();
};


#endif //HOT_TUB_CONTROLLER_HOTTUBUTILS_H