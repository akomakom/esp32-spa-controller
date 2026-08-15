
#include "HotTubUtils.h"




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
        u_int8_t otherOnValue = other->getOnStateForDependents();
        if (otherValue == otherOnValue) {
            result = ourValue;
        } else if (otherValue == SPECIAL_VALUE_ANY_GREATER_THAN_ZERO && otherOnValue > 0) {
            result = ourValue;
        }
        if (result == SPECIAL_VALUE_ANY_GREATER_THAN_ZERO) {
            result = otherOnValue; // use the same value for us
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
    return NULL; // no dependency configured
}


/*** SpaControlScheduler ***/
void
SpaControlScheduler::normalSchedule(u_int8_t percentageOfDayOnTime, u_int8_t numberOfTimesToRun, u_int8_t normalValueOn,
                                    u_int8_t normalValueOff) {
    // Convenience: express the old whole-day "% + segments" schedule as a single
    // time-of-day period (00:00 -> midnight). Used for code defaults and any caller
    // that still thinks in the old terms.
    checkBounds(normalValueOn);
    checkBounds(normalValueOff);
    int pct = std::max(0, std::min(100, (int)percentageOfDayOnTime));
    int segments = std::max(1, (int)numberOfTimesToRun);

    SpaSchedulePeriod p;
    p.startHour = 0;
    p.cycleMinutes = (u_int16_t)std::max(1, 1440 / segments);
    p.onMinutes = (u_int16_t)((long)p.cycleMinutes * pct / 100);
    p.onValue = normalValueOn;
    p.offValue = normalValueOff;
    setSchedule(&p, 1);
}

void SpaControlScheduler::setSchedule(const SpaSchedulePeriod* newPeriods, u_int8_t count) {
    if (count < 1) count = 1;
    if (count > MAX_SCHEDULE_PERIODS) count = MAX_SCHEDULE_PERIODS;

    SpaSchedulePeriod tmp[MAX_SCHEDULE_PERIODS];
    for (u_int8_t i = 0; i < count; i++) {
        tmp[i] = newPeriods[i];
        if (tmp[i].startHour > 23) tmp[i].startHour = 23;
        if (tmp[i].cycleMinutes < 1) tmp[i].cycleMinutes = 1;
        if (tmp[i].cycleMinutes > 1440) tmp[i].cycleMinutes = 1440;
        if (tmp[i].onMinutes > 1440) tmp[i].onMinutes = 1440;
        tmp[i].onValue  = std::min(max, std::max(min, tmp[i].onValue));
        tmp[i].offValue = std::min(max, std::max(min, tmp[i].offValue));
    }
    // sort by startHour ascending (tiny insertion sort)
    for (u_int8_t i = 1; i < count; i++) {
        SpaSchedulePeriod v = tmp[i];
        int j = i;
        while (j > 0 && tmp[j - 1].startHour > v.startHour) { tmp[j] = tmp[j - 1]; j--; }
        tmp[j] = v;
    }
    tmp[0].startHour = 0; // the first period always covers from midnight

    scheduleSettings.version = SCHEDULE_SETTINGS_VERSION;
    scheduleSettings.periodCount = count;
    for (u_int8_t i = 0; i < count; i++) scheduleSettings.periods[i] = tmp[i];
}

u_int8_t SpaControlScheduler::getCurrentPeriodIndex() {
    long secsToday = main_device_time->tm_hour * 3600L + main_device_time->tm_min * 60L + main_device_time->tm_sec;
    u_int8_t idx = 0;
    for (u_int8_t i = 0; i < scheduleSettings.periodCount; i++) {
        if ((long)scheduleSettings.periods[i].startHour * 3600L <= secsToday) {
            idx = i;
        } else {
            break; // periods are sorted by startHour; nothing later can match
        }
    }
    return idx;
}

void SpaControlScheduler::persist(const char* eepromKey) {
    scheduleSettings.version = SCHEDULE_SETTINGS_VERSION;
    if (!app_preferences.putBytes(eepromKey, &scheduleSettings, sizeof(scheduleSettings))) {
        Serial.print("Unable to persist schedule to key: ");
        Serial.println(eepromKey);
    } else {
        Serial.print("Persisted schedule to key: ");
        Serial.println(eepromKey);
    }
}
void SpaControlScheduler::load(const char* eepromKey) {
    // Read into a local and only adopt it if it is a full, current-version record.
    // Anything else (absent, or an older/short layout from a previous firmware) leaves
    // the code defaults in place -- this version intentionally does not migrate old
    // saved schedules; the user reconfigures via the new period editor.
    SpaScheduleSettings loaded;
    size_t got = app_preferences.getBytes(eepromKey, &loaded, sizeof(loaded));
    if (got == sizeof(loaded) && loaded.version == SCHEDULE_SETTINGS_VERSION
            && loaded.periodCount >= 1 && loaded.periodCount <= MAX_SCHEDULE_PERIODS) {
        scheduleSettings = loaded;
        // Clamp persisted values into the current [min,max] range (ranges can change
        // between firmware versions, e.g. the heater ceiling) so nothing trips later.
        for (u_int8_t i = 0; i < scheduleSettings.periodCount; i++) {
            scheduleSettings.periods[i].onValue  = std::min(max, std::max(min, scheduleSettings.periods[i].onValue));
            scheduleSettings.periods[i].offValue = std::min(max, std::max(min, scheduleSettings.periods[i].offValue));
        }
        Serial.print("Loaded schedule from key: ");
        Serial.println(eepromKey);
    } else {
        Serial.print("No current-version schedule for key (keeping defaults): ");
        Serial.println(eepromKey);
    }
}

void SpaControlScheduler::scheduleOverride(time_t startTime, time_t endTime, u_int8_t valueOverride) {
    checkBounds(valueOverride);
    this->overrideStartTime = startTime;
    this->overrideEndTime = endTime;
    this->overrideValue = valueOverride;
}

void SpaControlScheduler::cancelOverride() {
    // Set directly rather than via scheduleOverride(): the disabled sentinel is
    // intentionally out of [min,max] and would be rejected by checkBounds().
    this->overrideStartTime = now();
    this->overrideEndTime = now();
    this->overrideValue = SCHEDULER_DISABLED_VALUE;
}

void SpaControlScheduler::checkBounds(u_int8_t value) {
    if (value > max || value < min) {
        throw std::invalid_argument("Provided value for control is outside allowed range");
    }
}

u_int8_t SpaControlScheduler::getScheduledValue() {
    // Find the current time-of-day period, then apply its cycle: ON for the first
    // onMinutes of each cycleMinutes window (measured from the period's start).
    SpaSchedulePeriod& p = scheduleSettings.periods[getCurrentPeriodIndex()];

    if (p.cycleMinutes == 0 || p.onMinutes >= p.cycleMinutes) {
        return p.onValue;   // always on for this period
    }
    if (p.onMinutes == 0) {
        return p.offValue;  // always off for this period
    }
    long secsToday = main_device_time->tm_hour * 3600L + main_device_time->tm_min * 60L + main_device_time->tm_sec;
    long minsIntoPeriod = (secsToday - (long)p.startHour * 3600L) / 60L;
    if (minsIntoPeriod < 0) minsIntoPeriod = 0;
    long pos = minsIntoPeriod % p.cycleMinutes;
    return (pos < p.onMinutes) ? p.onValue : p.offValue;
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

time_t SpaControlScheduler::getOverrideScheduleElapsedTime() {
    if (overrideStartTime <= now()) {
        return std::min((time_t)(overrideEndTime - overrideStartTime), now() - overrideStartTime);
    }
    return 0;
}


void SpaControlScheduler::updateConfigJsonString() {
    jsonConfig.clear();
    jsonConfig["overrideDefaultDurationSeconds"] = scheduleSettings.overrideDefaultDurationSeconds;
    jsonConfig["min"] = min;
    jsonConfig["max"] = max;
    jsonConfig["maxPeriods"] = MAX_SCHEDULE_PERIODS;
    jsonConfig["gated"] = dependencyGatedBySchedule; // ozone: schedule arms, not runs
    JsonArray periods = jsonConfig.createNestedArray("periods");
    for (u_int8_t i = 0; i < scheduleSettings.periodCount; i++) {
        JsonObject o = periods.createNestedObject();
        o["startHour"]    = scheduleSettings.periods[i].startHour;
        o["onMinutes"]    = scheduleSettings.periods[i].onMinutes;
        o["cycleMinutes"] = scheduleSettings.periods[i].cycleMinutes;
        o["onValue"]      = scheduleSettings.periods[i].onValue;
        o["offValue"]     = scheduleSettings.periods[i].offValue;
    }
    serializeJson(jsonConfig, configString);
}


/*** SpaControl ***/
SpaControl::SpaControl(const char *name, const char *type) {
    this->name = name;
    this->type = type;
    sprintf(this->persistKeyScheduler, "ctrl_sch_%s", name);
}

void SpaControl::toggle() {
    scheduleOverride(
            now(),
            now() + ((getOverrideScheduleRemainingTime() > 0) ?
                getOverrideScheduleRemainingTime() : scheduleSettings.overrideDefaultDurationSeconds),
            getNextValue());
//    Serial.println("PARENT!!! Value after toggle is ");
//    Serial.println(getEffectiveValue());
}

void SpaControl::applyUserValue(time_t relStart, time_t relEnd, u_int8_t value) {
    // Default: a temporary override that reverts to the normal schedule afterwards.
    scheduleOverride(now() + relStart, now() + relEnd, value);
}

u_int8_t SpaControl::getEffectiveValue() {
//    Serial.printf("GetEffectiveValue %s\n", name);
    if (isOverrideScheduleEnabled()) {
        return getOverrideValue();
    }
//    Serial.printf("GetEffectiveValue 2 %s\n", name);
    u_int8_t dependencyValue = getDependencyValue();
    if (dependencyGatedBySchedule) {
        // The schedule only ARMS this control (e.g. ozone): it turns on solely when its
        // dependency (the pump) is active AND its own schedule is on for this period. It
        // is never turned on by the schedule alone -- an armed-but-pump-off period is off,
        // and an unarmed period is off even while the pump runs.
        if (dependencyValue != SpaControlDependencies::SPECIAL_RETURN_VALUE_NOT_IN_EFFECT
                && getScheduledValue() > 0) {
            return dependencyValue;
        }
        return 0;
    }
    if (dependencyValue != SpaControlDependencies::SPECIAL_RETURN_VALUE_NOT_IN_EFFECT) {
        return dependencyValue;
    }
//    Serial.printf("GetEffectiveValue done %s", name);
    return getScheduledValue();
}

u_int8_t SpaControl::getOnState() {
    if (spaWinterized) return 0; // forced off, ignoring schedules/overrides
    return getEffectiveValue();
}

u_int8_t SpaControl::getOnStateForDependents() {
    return getOnState();
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
//    Serial.printf("Applying outputs for %s", name);
    digitalWrite(pin, getOnState() ? HIGH : LOW); // getOnState() honors winterized mode
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
//    Serial.printf("Applying outputs for %s", name);
    switch (getOnState()) { // getOnState() honors winterized mode (0 when winterized)
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

SensorBasedControl::SensorBasedControl(const char *name, u_int8_t pin, u_int8_t sensorIndex, u_int8_t swing, time_t postShutdownOnTime, TemperatureUtils* temps) : SpaControl(name, "sensor-based") {
    this->pin = pin;
    this->sensorIndex = sensorIndex;
    // Default limits (native unit is Fahrenheit, better integer resolution):
    this->min = 0;
    this->max = 104; // a safe hot-tub ceiling
    this->swing = swing;
    this->postShutdownOnTime = postShutdownOnTime;
    this->temperatureUtils = temps;

    pinMode(pin, OUTPUT);
}

/**
 * Dependents don't care obout our temperature threshold, they care whether we are on or off
 * @return
 */
u_int8_t SensorBasedControl::getOnState() {
    if (spaWinterized) return 0; // forced off, ignoring the setpoint/schedule
    // Don't just flip/flop any time temperature is below threshold.
    // Instead:
    //  if currently off and we are colder than (threshold - swing), turn on
    //  if currently on and we are hotter than (threshold + swing), turn off.
    //  if within threshold+/- swing, stay where you are

    // Delta to where we want to be, positive is too hot, negative is too cold:
    u_int8_t effectiveValue = getEffectiveValue();
    float delta = temperatureUtils->getTempF(this->sensorIndex) - (float)effectiveValue;
    if (effectiveValue == max && delta >= 0) {
        // We must never exceed max, this is a safety feature.
        // An appliance may not be capable of going any higher and will be stuck ON forever
        // Do not apply deadband logic in this case.
        slowFlipState = false;
    } else if (std::abs(delta) > swing) {
        // Are we outside of the swing deadband (setpoint-swing to setpoint+swing)?
        // too hot or too cold and outside deadband, turn on or off regardless:
        slowFlipState = (delta > 0) ? false : true;
    } else {
        // within deadband, do not change state
    }
    return slowFlipState ? 1 : 0;
}
/**
 * Implements postShutdownStayOn logic to extract heat from heater by keeping pump on
 * @return same as getOnState unless postShutdown delay is in effect
 */
u_int8_t SensorBasedControl::getOnStateForDependents() {
    if (spaWinterized) return 0; // forced off; don't hold dependents on via post-shutdown delay
    getOnState();

    if (slowFlipState == 1) {
        // save the stay-on-until time
        postShutdownStayOnUntil = now() + postShutdownOnTime;
    } else {
        // apply post shutdown delay
        if (now() < postShutdownStayOnUntil) {
            return 1;
        }
    }
    return slowFlipState ? 1 : 0;
}

void SensorBasedControl::applyOutputs() {
//    Serial.printf("Applying outputs for %s", name);
    digitalWrite(pin, getOnState());
}

void SensorBasedControl::setSetpoint(u_int8_t value) {
    // A single "master" setpoint: apply it as the on-value of every scheduled period
    // (per-period setpoints are edited through the schedule editor instead). Persist so
    // it survives the override window and reboots.
    value = std::min(max, std::max(min, value));
    for (u_int8_t i = 0; i < scheduleSettings.periodCount; i++) {
        scheduleSettings.periods[i].onValue = value;
    }
    // Clear any lingering override so the new persistent setpoint takes effect now.
    cancelOverride();
    persist();
}

void SensorBasedControl::applyUserValue(time_t relStart, time_t relEnd, u_int8_t value) {
    // Temporary setpoint override for the given window; when it expires the heater
    // reverts to the scheduled setpoint (normalValueOn) — same model as other
    // controls, so the display shows a countdown (ORT). The persistent/scheduled
    // setpoint is configured from the web schedule dialog (normalValueOn).
    scheduleOverride(now() + relStart, now() + relEnd, value);
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
        // default override time
        control->jsonStatus["DO"] = control->scheduleSettings.overrideDefaultDurationSeconds;
        control->jsonStatus["type"] = control->type;
        control->jsonStatus["ORT"] = control->getOverrideScheduleRemainingTime();
        control->jsonStatus["val_o"] = control->getOnState();
        control->jsonStatus["val_d"] = control->getOnStateForDependents();
        if (control->getDependentControl() != NULL) {
            control->jsonStatus["depctl"] = control->getDependentControl()->name;
        }
    }

    // Native unit is Fahrenheit; the web UI converts for display per "temp_unit".
    jsonStatusMetrics["temp"] = temperatureUtils.getTempF(0);
    jsonStatusMetrics["temp_unit"] = app_preferences.getUChar("temp_unit", 0);
    jsonStatusMetrics["winterized"] = spaWinterized;
    jsonStatusMetrics["time"] = mktime(main_device_time);
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

    // Defaults until UI configures these values (native unit is Fahrenheit):
    // Sane new-model default: pump on Low for 5 min out of every 30, all day.
    { SpaSchedulePeriod pp; pp.startHour = 0; pp.cycleMinutes = 30; pp.onMinutes = 5; pp.onValue = 1; pp.offValue = 0; pump->setSchedule(&pp, 1); }
    // Always on; heat to a sane default setpoint (100F), keep above freezing (40F)
    // when scheduled off. The setpoint is persistent and editable from the UI.
    heater->normalSchedule(100, 1, 100, std::max(heater->min, (u_int8_t)40)); 
    heater->scheduleSettings.overrideDefaultDurationSeconds = 3600 * 2; // different default

    pump->neededBy(heater, 1, 1);

    ozone->lockedTo(pump, SpaControlDependencies::SPECIAL_VALUE_ANY_GREATER_THAN_ZERO, 1);
    // Ozone follows the pump, but only during periods its own schedule marks "on".
    // Default: armed all day (= today's behaviour) until off-periods are scheduled.
    ozone->dependencyGatedBySchedule = true;
    ozone->normalSchedule(100, 1, 1, 0);
    ozone->scheduleSettings.overrideDefaultDurationSeconds = 3600; //default

    Serial.println("About to iterate and load settings");
    // apply saved preferences
    // JSON init
    for (SpaControl *control: controls) {
        try {
            control->load();
        } catch (std::exception &e) {
            // Corrupt or out-of-range persisted settings must not brick the controller.
            Serial.printf("Failed to load settings for %s (%s), keeping defaults\n", control->name, e.what());
        }
        control->jsonStatus = jsonStatusControls.createNestedObject();
    }
    temperatureUtils.setup();
}

void SpaStatus::loop() {
    for (SpaControl *control: controls) {
        control->applyOutputs();
    }
    temperatureUtils.loop();
}
