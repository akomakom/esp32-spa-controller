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
#define STATUS_LENGTH 400

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
     * @return true if a schedule is in effect, whether normal or override
     */
    bool isScheduleEnabled();

    /**
     * @return scheduled value if schedule is enabled (see isScheduleEnabled)
     * If override is enabled, returns overrideValue
     * if only normal schedule is enabled, returns normalValueOn or normalValueOff
     * If neither is enabled, returns SCHEDULER_DISABLED_VALUE
     */
    u_int8_t getScheduledValue();

private:
    static const u_int8_t SCHEDULER_DISABLED_VALUE = -1;
    u_int8_t percentageOfDayOnTime = SCHEDULER_DISABLED_VALUE;
    u_int8_t numberOfTimesToRun = SCHEDULER_DISABLED_VALUE;
    u_int8_t normalValueOn = SCHEDULER_DISABLED_VALUE;
    u_int8_t normalValueOff = SCHEDULER_DISABLED_VALUE;

    u_int8_t overrideStartTime = now();
    u_int8_t overrideEndTime = now();
    u_int8_t overrideValue = SCHEDULER_DISABLED_VALUE;

    bool isNormalScheduleEnabled();
    bool isOverrideScheduleEnabled();

};

class SpaControl {
    const int DEFAULT_MIN = 0;
    const int DEFAULT_MAX = 1;
public:
    SpaControl(const char *name, const char *type);

    virtual void toggle();

    virtual void set(u_int8_t value);

    virtual void applyOutputs();

    const char *name;
    u_int8_t min = DEFAULT_MIN;
    u_int8_t max = DEFAULT_MAX;
    const char *type;
    u_int8_t value = 0;
    JsonObject jsonStatus;

private:
    void init(const char *name, u_int8_t min, u_int8_t max);

protected:
    void incrementValue();
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

private:
    SimpleSpaControl *power;
    SimpleSpaControl *speed;
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