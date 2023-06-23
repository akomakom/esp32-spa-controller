#include <sys/types.h>

#ifndef __UTILS_H__
#define __UTILS_H__

#include "pgmspace.h"
#include <stdexcept>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

// needs to be large enough to allocate json document and string representation
#define STATUS_LENGTH 500

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
     * @return scheduled value if schedule is enabled
     * If override is enabled, returns overrideValue
     * Returns normalValueOn or normalValueOff otherwise
     */
    u_int8_t getScheduledValue();

private:
    static const u_int8_t SCHEDULER_DISABLED_VALUE = -1;
    // default is always off:
    u_int8_t percentageOfDayOnTime = 0;
    u_int8_t numberOfTimesToRun = 1; // stay on
    u_int8_t normalValueOn = 1;
    u_int8_t normalValueOff = 0;

    time_t overrideStartTime = now();
    time_t overrideEndTime = now();
    u_int8_t overrideValue = SCHEDULER_DISABLED_VALUE;

};

class SpaControl : public SpaControlScheduler {
    const int DEFAULT_MIN = 0;
    const int DEFAULT_MAX = 1;
public:
    SpaControl(const char *name, const char *type);

    virtual void toggle();

    /**
     * Change the local (not scheduler) value.
     * There may be a scheduler on the control that can override this value
     * Changing that value should be done via the scheduler instance
     * @param value
     */
    virtual void set(u_int8_t value);

    /**
     *
     * @return either this->value or this->scheduler value if scheduler is in effect
     */
    virtual u_int8_t getEffectiveValue();

    virtual void applyOutputs();

    const char *name;
    u_int8_t min = DEFAULT_MIN;
    u_int8_t max = DEFAULT_MAX;
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

    virtual void set(u_int8_t value);

    virtual void applyOutputs();

    u_int8_t pinPower;
    u_int8_t pinSpeed;
};

class SpaStatus {
private:
    StaticJsonDocument<STATUS_LENGTH> jsonStatus;
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

    char statusString[STATUS_LENGTH];

    void updateStatusString();

//    SpaControl *controls[3] = {blower, heater, ozone};
    SpaControl *controls[4] = {pump, blower, heater, ozone};

//    SpaControl *controls[5] = {pump, pump_fast, blower, heater, ozone};

    SpaControl *findByName(const char *name);

    void applyControls();

};


#endif