#include <sys/types.h>
#include "esp32-hal-gpio.h"
#include "HotTubUtils.h"

/*** SpaControlScheduler ***/
void
SpaControlScheduler::normalSchedule(u_int8_t percentageOfDayOnTime, u_int8_t numberOfTimesToRun, u_int8_t normalValueOn,
                                    u_int8_t normalValueOff) {
    this->percentageOfDayOnTime = percentageOfDayOnTime;
    this->numberOfTimesToRun = numberOfTimesToRun;
    this->normalValueOn = normalValueOn;
    this->normalValueOff = normalValueOff;
}

void SpaControlScheduler::scheduleOverride(time_t startTime, time_t endTime, u_int8_t valueOverride) {
    this->overrideStartTime = startTime;
    this->overrideEndTime = endTime;
    this->overrideValue = valueOverride;
}

void SpaControlScheduler::cancelOverride() {
    scheduleOverride(now(), now(), SCHEDULER_DISABLED_VALUE);
}

bool SpaControlScheduler::isScheduleEnabled() {
    return isNormalScheduleEnabled() || isOverrideScheduleEnabled();
}

u_int8_t SpaControlScheduler::getScheduledValue() {
    if (isOverrideScheduleEnabled()) {
        return overrideValue;
    }
    if (isNormalScheduleEnabled()) {
        // calculate percentage

        // Percentage of each on and off segment:
        float onLengthPercentage = percentageOfDayOnTime / numberOfTimesToRun;
        float offLengthPercentage = (100 - percentageOfDayOnTime) / numberOfTimesToRun;

        // which segment are we in currently?
        float currentPercentageOfDay = 100 * elapsedSecsToday(now()) / SECS_PER_DAY;

        for (u_int8_t index = 1; index <= numberOfTimesToRun; index++) {
            if ((onLengthPercentage * index) > currentPercentageOfDay) {
                return normalValueOn;
            } else if ((onLengthPercentage * index + offLengthPercentage * index) > currentPercentageOfDay) {
                return normalValueOff;
            }
        }
    }

    return SCHEDULER_DISABLED_VALUE;
}

bool SpaControlScheduler::isNormalScheduleEnabled() {
    return (percentageOfDayOnTime > 0 && numberOfTimesToRun > 0 && normalValueOn != SCHEDULER_DISABLED_VALUE &&
            normalValueOff != SCHEDULER_DISABLED_VALUE);
}

bool SpaControlScheduler::isOverrideScheduleEnabled() {
    return (overrideStartTime <= now() && overrideEndTime > now() && overrideValue != SCHEDULER_DISABLED_VALUE);
}

/*** SpaControl ***/
SpaControl::SpaControl(const char *name, const char *type) {
    this->name = name;
    this->type = type;
}

void SpaControl::toggle() {
    incrementValue();
    Serial.println("PARENT!!! Value after unimplemented toggle is ");
    Serial.println(value);
}

void SpaControl::set(u_int8_t value) {
    if (value < min || value > max) {
        throw std::invalid_argument("Provided value for control is outside allowed range");
    }
    this->value = value; // base class simply accepts the value
}

void SpaControl::applyOutputs() {}

void SpaControl::incrementValue() {
    if (value >= max) {
        set(0);
    } else {
        set(value + 1);
    }
    Serial.println("Value after toggle is ");
    Serial.println(value);
}


/*** SimpleSpaControl ***/

SimpleSpaControl::SimpleSpaControl(const char *name, u_int8_t pin) : SpaControl(name, "off-on") {
    this->pin = pin;
    pinMode(pin, OUTPUT);
}

void SimpleSpaControl::toggle() {
    SpaControl::toggle();
}

void SimpleSpaControl::applyOutputs() {
    digitalWrite(pin, value ? HIGH : LOW);
}

TwoSpeedSpaControl::TwoSpeedSpaControl(const char *name, u_int8_t pin_power, u_int8_t pin_speed) : SpaControl(name,
                                                                                                              "off-low-high") {
    this->power = new SimpleSpaControl(name, pin_power);
    this->speed = new SimpleSpaControl(name, pin_speed);
    this->max = 2; // 0,1,2 - off/low/high
}

/**
 * States are (Power/Speed), in order:
 * 0/0
 * 1/0
 * 1/1
 * (repeat)
 */
void TwoSpeedSpaControl::toggle() {
    incrementValue();
}

void TwoSpeedSpaControl::set(u_int8_t value) {
    SpaControl::set(value);  // this will check the argument
    switch (value) {
        case 0:
            this->power->value = 0;
            this->speed->value = 0;
            break;
        case 1:
            this->power->value = 1;
            this->speed->value = 0;
            break;
        case 2:
            this->power->value = 1;
            this->speed->value = 1;
            break;
    }
}

void TwoSpeedSpaControl::applyOutputs() {
    this->power->applyOutputs();
    this->speed->applyOutputs();
}


/*** SpaStatus ***/

SpaStatus::SpaStatus() {
    for (SpaControl *control: controls) {
        control->jsonStatus = jsonStatusControls.createNestedObject(control->name);
    }
}

void SpaStatus::updateStatusString() {
    for (SpaControl *control: controls) {
        control->jsonStatus["value"] = control->value;
        control->jsonStatus["min"] = control->min;
        control->jsonStatus["max"] = control->max;
        control->jsonStatus["type"] = control->type;
    }

    jsonStatusMetrics["temp"] = 100;
    serializeJson(jsonStatus, statusString);
}

SpaControl *SpaStatus::findByName(const char *name) {
    for (SpaControl *control: controls) {
        if (strcmp(control->name, name) == 0) {
            return control;
        }
    }
    Serial.println("No control found with name: ");
    Serial.println(name);
    throw std::invalid_argument("Control not found with this name");
}

void SpaStatus::applyControls() {
    for (SpaControl *control: controls) {
        control->applyOutputs();
    }
}
