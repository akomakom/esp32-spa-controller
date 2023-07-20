
#include "HotTubUtils.h"



extern Preferences app_preferences = Preferences();

/*** SpaControlDependencies ***/

void SpaControlDependencies::neededBy(SpaControl *otherControl, u_int8_t otherControlValue, u_int8_t ourValue) {
    this->neededByOtherControl = otherControl;
    this->neededByOtherControlValue = otherControlValue;
    this->neededByOurValue = ourValue;
}

void SpaControlDependencies::lockedTo(SpaControl *otherControl, u_int8_t otherControlValue, u_int8_t ourValue) {
    this->lockedToOtherControl = otherControl;
    this->lockedToOtherControlValue = otherControlValue;
    this->lockedToOurValue = ourValue;
}

/**
 * Code must be kept in sync with isDependencyInEffect 
 */
u_int8_t SpaControlDependencies::getDependencyValue(SpaControl* other, u_int8_t otherValue, u_int8_t ourValue) {
    u_int8_t result = SPECIAL_RETURN_VALUE_NOT_IN_EFFECT;
    if (other != NULL) {
        if (otherValue == other->getEffectiveValueForDependents()) {
            result = ourValue;
        } else if (otherValue == SPECIAL_VALUE_ANY_GREATER_THAN_ZERO && other->getEffectiveValueForDependents() > 0) {
            result = ourValue;
        }
        if (result == SPECIAL_VALUE_ANY_GREATER_THAN_ZERO) {
            result = other->getEffectiveValueForDependents(); // use the same value for us
        }
    }
    return result;
}

u_int8_t SpaControlDependencies::getDependencyValue() {
    u_int8_t result = getDependencyValue(lockedToOtherControl, lockedToOtherControlValue, lockedToOurValue);
    if (result == SPECIAL_RETURN_VALUE_NOT_IN_EFFECT) {
        result = getDependencyValue(neededByOtherControl, neededByOtherControlValue, neededByOurValue);
    }

    return result;
}

SpaControl *SpaControlDependencies::getDependentControl() {
    if (lockedToOtherControl != NULL) {
        return lockedToOtherControl;
    } else if (neededByOtherControl != NULL) {
        return neededByOtherControl;
    }
}


/*** SpaControlScheduler ***/
void
SpaControlScheduler::normalSchedule(u_int8_t percentageOfDayOnTime, u_int8_t numberOfTimesToRun, u_int8_t normalValueOn,
                                    u_int8_t normalValueOff) {
    checkBounds(normalValueOn);
    checkBounds(normalValueOff);
    normalSettings.percentageOfDayOnTime = std::max(0, std::min(100, (int)percentageOfDayOnTime));
    normalSettings.numberOfTimesToRun = std::max(1, (int)numberOfTimesToRun);
    normalSettings.normalValueOn = normalValueOn;
    normalSettings.normalValueOff = normalValueOff;

    // precalculate normal schedule variables
    float onLengthPercentage = (float)normalSettings.percentageOfDayOnTime / (float)normalSettings.numberOfTimesToRun;
    float offLengthPercentage = (float)(100 - normalSettings.percentageOfDayOnTime) / (float)normalSettings.numberOfTimesToRun;
    onOffLengthPercentage = onLengthPercentage + offLengthPercentage;
    // how ON compares with OFF (how far is the divider), as a 0-1 fraction, <0.5 is on, >0.5 is off
    onVsOff = onLengthPercentage / onOffLengthPercentage;

}

void SpaControlScheduler::persist(const char* eepromKey) {
    // persist this to EEPROM
    if (!app_preferences.putBytes(eepromKey, &normalSettings, sizeof(normalSettings))) {
        Serial.print("Unable to persist setting to key: ");
        Serial.println(eepromKey);
    } else {
        Serial.print("Persisted setting to key: ");
        Serial.println(eepromKey);
    }
}
void SpaControlScheduler::load(const char* eepromKey) {
    SpaSchedulerNormalSettings settings;
    Serial.print("Retrieving values from eeprom for key");
    Serial.println(eepromKey);

    if (app_preferences.getBytes(eepromKey, &settings, sizeof(settings))) {
        // apply so that math can be precalculated
        Serial.println("Retrieved values from eeprom, eg");
        Serial.println(settings.percentageOfDayOnTime);
        normalSchedule(settings.percentageOfDayOnTime, settings.numberOfTimesToRun, settings.normalValueOn, settings.normalValueOff);
    }
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
    return (fractionOfOnOffUnit > onVsOff) ? normalSettings.normalValueOff : normalSettings.normalValueOn;
}

bool SpaControlScheduler::isOverrideScheduleEnabled() {
    return (getOverrideScheduleRemainingTime() > 0 && overrideValue != SCHEDULER_DISABLED_VALUE);
}

u_int8_t SpaControlScheduler::getOverrideValue() {
    return overrideValue;
}

time_t SpaControlScheduler::getOverrideScheduleRemainingTime() {
    if (overrideStartTime <= now()) {
        return std::max((time_t)0, overrideEndTime - now());
    }
    return 0;
}


void SpaControlScheduler::updateConfigJsonString() {
    jsonConfig["percentageOfDayOnTime"] = normalSettings.percentageOfDayOnTime;
    jsonConfig["numberOfTimesToRun"] = normalSettings.numberOfTimesToRun;
    jsonConfig["normalValueOn"] = normalSettings.normalValueOn;
    jsonConfig["normalValueOff"] = normalSettings.normalValueOff;
    serializeJson(jsonConfig, configString);
}


/*** SpaControl ***/
SpaControl::SpaControl(const char *name, const char *type) {
    this->name = name;
    this->type = type;
    sprintf(this->persistKeyScheduler, "ctrl_sch_%s", name);
}

void SpaControl::toggle() {
    // TODO: decide on defaults ore take as input
    scheduleOverride(now(), now() + 20 * SECS_PER_MIN, getNextValue());
    Serial.println("PARENT!!! Value after toggle is ");
    Serial.println(getEffectiveValue());
}

u_int8_t SpaControl::getEffectiveValue() {
    if (isOverrideScheduleEnabled()) {
        return getOverrideValue();
    }
    u_int8_t dependencyValue = getDependencyValue();
    if (dependencyValue != SpaControlDependencies::SPECIAL_RETURN_VALUE_NOT_IN_EFFECT) {
        return dependencyValue;
    }
    return getScheduledValue();
}

u_int8_t SpaControl::getEffectiveValueForDependents() {
    return getEffectiveValue();
}

void SpaControl::applyOutputs() {}

u_int8_t SpaControl::getNextValue() {
    if (getEffectiveValue() >= max) {
        return 0;
    } else {
        return getEffectiveValue() + 1;
    }
}

void SpaControl::persist() {
    SpaControlScheduler::persist(persistKeyScheduler);
}

void SpaControl::load() {
    Serial.print("Loading settings for control ");
    Serial.println(name);
    SpaControlScheduler::load(persistKeyScheduler);
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

SensorBasedControl::SensorBasedControl(const char *name, u_int8_t pin, u_int8_t sensorIndex, u_int8_t swing, TemperatureUtils* temps) : SpaControl(name, "sensor-based") {
    this->pin = pin;
    this->sensorIndex = sensorIndex;
    this->min = 60; // using Fahrenheit because there is better resolution with integers
    this->max = 105;
    this->swing = swing;
    this->temperatureUtils = temps;

    pinMode(pin, OUTPUT);
}

/**
 * Dependents don't care obout our temperature threshold, they care whether we are on or off
 * @return
 */
u_int8_t SensorBasedControl::getEffectiveValueForDependents() {
    // Don't just flip/flop any time temperature is below threshold.
    // Instead:
    //  if currently off and we are colder than (threshold - swing), turn on
    //  if currently on and we are hotter than (threshold + swing), turn off.
    //  if within threshold+/- swing, stay where you are

    // Delta to where we want to be, positive is too hot, negative is too cold:
    float delta = temperatureUtils->getTempF(this->sensorIndex) - (float)getEffectiveValue();
    // Are we outside of the swing deadband (setpoint-swing to setpoint+swing)?
    if (std::abs(delta) > swing) {
        // too hot or too cold and outside deadband, turn on or off regardless:
        slowFlipState = (delta > 0) ? false : true;
    } else {
        // within deadband, do not change state
    }
    return slowFlipState ? 1 : 0;
}

void SensorBasedControl::applyOutputs() {
    digitalWrite(pin, getEffectiveValueForDependents());
}


/*** SpaStatus ***/

SpaStatus::SpaStatus() {

}

void SpaStatus::updateStatusString() {
    for (SpaControl *control: controls) {
        control->jsonStatus["name"] = control->name;
        control->jsonStatus["value"] = control->getEffectiveValue();
        control->jsonStatus["min"] = control->min;
        control->jsonStatus["max"] = control->max;
        control->jsonStatus["type"] = control->type;
        control->jsonStatus["ORT"] = control->getOverrideScheduleRemainingTime();
        if (control->getDependentControl() != NULL) {
            control->jsonStatus["depctl"] = control->getDependentControl()->name;
            control->jsonStatus["depval"] = control->getDependencyValue();
        }
    }

    jsonStatusMetrics["temp"] = temperatureUtils.getTempF(0);
    jsonStatusMetrics["time"] = timeClient->getFormattedTime();
    jsonStatusMetrics["uptime"] = esp_timer_get_time() / 1000000;
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

void SpaStatus::setup() {
    timeClient = new NTPClient(ntpUDP, "pool.ntp.org", 0, 3600000);
    timeClient->begin();

    // Defaults:
    pump->normalSchedule(50, 2, 1, 0); // TODO: Save preferences and build a UI to modify
    heater->normalSchedule(100, 1, heater->max, heater->min); // always on, set to max temp
    pump->neededBy(heater, 1, 1);
    ozone->lockedTo(pump, SpaControlDependencies::SPECIAL_VALUE_ANY_GREATER_THAN_ZERO, 1);

    Serial.println("About to iterate and load settings");
    // apply saved preferences
    // JSON init
    for (SpaControl *control: controls) {
        control->load();
        control->jsonStatus = jsonStatusControls.createNestedObject();
    }
    temperatureUtils.setup();
}

void SpaStatus::loop() {
    timeClient->update();
    for (SpaControl *control: controls) {
        control->applyOutputs();
    }
    temperatureUtils.loop();
}
