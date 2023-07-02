#include <sys/types.h>
#include "esp32-hal-gpio.h"
#include "HotTubUtils.h"
#include <algorithm>

/*** SpaControlScheduler ***/
void
SpaControlScheduler::normalSchedule(u_int8_t percentageOfDayOnTime, u_int8_t numberOfTimesToRun, u_int8_t normalValueOn,
                                    u_int8_t normalValueOff) {
    checkBounds(normalValueOn);
    checkBounds(normalValueOff);
    this->percentageOfDayOnTime = std::max(0, std::min(100, (int)percentageOfDayOnTime));
    this->numberOfTimesToRun = std::max(1, (int)numberOfTimesToRun);
    this->normalValueOn = normalValueOn;
    this->normalValueOff = normalValueOff;

    // precalculate normal schedule variables
    float onLengthPercentage = (float)percentageOfDayOnTime / (float)numberOfTimesToRun;
    float offLengthPercentage = (float)(100 - percentageOfDayOnTime) / (float)numberOfTimesToRun;
    onOffLengthPercentage = onLengthPercentage + offLengthPercentage;
    // how ON compares with OFF (how far is the divider), as a 0-1 fraction, <0.5 is on, >0.5 is off
    onVsOff = onLengthPercentage / onOffLengthPercentage;
}

void SpaControlScheduler::scheduleOverride(time_t startTime, time_t endTime, u_int8_t valueOverride) {
    checkBounds(valueOverride);
    this->overrideStartTime = startTime;
    this->overrideEndTime = endTime;
    this->overrideValue = valueOverride;
}

void SpaControlScheduler::cancelOverride() {
    scheduleOverride(now(), now(), SCHEDULER_DISABLED_VALUE);
}

void SpaControlScheduler::checkBounds(u_int8_t value) {
    if (value > max || value < min) {
        throw std::invalid_argument("Provided value for control is outside allowed range");
    }
}

u_int8_t SpaControlScheduler::getScheduledValue() {
    if (isOverrideScheduleEnabled()) {
        return overrideValue;
    }
    // trust normal schedule.
    // Normal schedule if a series of on-off time segments configured as a percentage of a day's length

    // Percentage of each on and off segment:
    // TODO: precalculate on normalSchedule() call
    // length of each unit as a percentage of day length

    // How far are we into the day (since midnight), in percentages?
    float currentPercentageOfDay = (float)100 * elapsedSecsToday(now()) / SECS_PER_DAY;
    // which segment are we in currently?
    // how many on+off time units into the day are we?
    // eg we are 2.36 on/off segments into the day
    float onOffUnitCount = currentPercentageOfDay / onOffLengthPercentage;
    float fractionOfOnOffUnit =  onOffUnitCount-(long)onOffUnitCount; //leave fraction only, eg 0.36

    // Are we past the on->off divider in this on-then-off time unit?
    return (fractionOfOnOffUnit > onVsOff) ? normalValueOff : normalValueOn;
}

bool SpaControlScheduler::isOverrideScheduleEnabled() {
    return (getOverrideScheduleRemainingTime() > 0 && overrideValue != SCHEDULER_DISABLED_VALUE);
}

time_t SpaControlScheduler::getOverrideScheduleRemainingTime() {
    if (overrideStartTime <= now()) {
        return std::max((time_t)0, overrideEndTime - now());
    }
    return 0;
}

/*** SpaControl ***/
SpaControl::SpaControl(const char *name, const char *type) {
    this->name = name;
    this->type = type;
}

void SpaControl::toggle() {
    // TODO: decide on defaults ore take as input
    scheduleOverride(now(), now() + 2 * SECS_PER_MIN, getNextValue());
    Serial.println("PARENT!!! Value after toggle is ");
    Serial.println(getEffectiveValue());
}

u_int8_t SpaControl::getEffectiveValue() {
    return getScheduledValue();
}

void SpaControl::applyOutputs() {}

u_int8_t SpaControl::getNextValue() {
    if (getEffectiveValue() >= max) {
        return 0;
    } else {
        return getEffectiveValue() + 1;
    }
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
    digitalWrite(pin, getEffectiveValue() ? HIGH : LOW);
}

TwoSpeedSpaControl::TwoSpeedSpaControl(const char *name, u_int8_t pin_power, u_int8_t pin_speed) : SpaControl(name, "off-low-high") {
    this->pinPower = pin_power;
    this->pinSpeed = pin_speed;
    this->max = 2; // 0,1,2 - off/low/high
    pinMode(pin_power, OUTPUT);
    pinMode(pin_speed, OUTPUT);
}
void TwoSpeedSpaControl::toggle() {
    SpaControl::toggle();
}

void TwoSpeedSpaControl::applyOutputs() {
    switch (getEffectiveValue()) {
        case 0:
            digitalWrite(pinPower, 0);
            digitalWrite(pinSpeed, 0);
            break;
        case 1:
            digitalWrite(pinPower, 1);
            digitalWrite(pinSpeed, 0);
            break;
        case 2:
            digitalWrite(pinPower, 1);
            digitalWrite(pinSpeed, 1);
            break;
    }
}


/*** SpaStatus ***/

SpaStatus::SpaStatus() {
    pump->normalSchedule(50, 2, 1, 0); // TODO: Save preferences and build a UI to modify
    for (SpaControl *control: controls) {
        control->jsonStatus = jsonStatusControls.createNestedObject();
    }
}

void SpaStatus::updateStatusString() {
    for (SpaControl *control: controls) {
        control->jsonStatus["name"] = control->name;
        control->jsonStatus["value"] = control->getEffectiveValue();
        control->jsonStatus["min"] = control->min;
        control->jsonStatus["max"] = control->max;
        control->jsonStatus["type"] = control->type;
        control->jsonStatus["ORT"] = control->getOverrideScheduleRemainingTime();
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

    char buffer [200];
    sprintf(buffer, "Control not found with name : %s", name);
    throw std::invalid_argument(buffer);
}

void SpaStatus::applyControls() {
    for (SpaControl *control: controls) {
        control->applyOutputs();
    }
}
