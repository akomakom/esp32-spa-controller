#include <sys/types.h>
#include "esp32-hal-gpio.h"
#include "HotTubUtils.h"

/*** SpaControl ***/

SpaControl::SpaControl(const char* name, u_int8_t min, u_int8_t max) {
  init(name, min, max);
}
SpaControl::SpaControl(const char* name) {
  init(name, DEFAULT_MIN, DEFAULT_MAX);
}

void SpaControl::init(const char *name, u_int8_t min, u_int8_t max) {
    this->name = name;
    this->min = min;
    this->max = max;
}

void SpaControl::toggle() {
    incrementValue();
    Serial.println("PARENT!!! Value after unimplemented toggle is "); Serial.println(value);
}
void SpaControl::applyOutputs() {}
void SpaControl::incrementValue() {
    value++;
    if (value > max) {
        value = 0;
    }
    Serial.println("Value after toggle is "); Serial.println(value);
}


/*** SimpleSpaControl ***/

SimpleSpaControl::SimpleSpaControl(const char *name, u_int8_t pin) : SpaControl(name) {
    // TODO: re-enable
    this->pin = pin;
    pinMode(pin, OUTPUT);
    Serial.print("Setting pinmode for ");
    Serial.print(name);
    Serial.print(" to ");
    Serial.println(pin);
}

SimpleSpaControl::SimpleSpaControl(const char* name, u_int8_t pin, u_int8_t min, u_int8_t max) : SpaControl(name, min, max) {
  pinMode(pin, OUTPUT);
}

void SimpleSpaControl::toggle() {
    SpaControl::toggle();
}

void SimpleSpaControl::applyOutputs() {
    //TDOD: re-enable
  digitalWrite(pin, value ? HIGH : LOW);
}

TwoSpeedSpaControl::TwoSpeedSpaControl(const char *name, u_int8_t pin_power, u_int8_t pin_speed) : SpaControl(name) {
    this->name = name;
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
    switch(value) {
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
}

void SpaStatus::updateStatusString() {
  for (SpaControl* control: controls) {
    jsonStatusControls[control->name] = control->value;
  }
  
  jsonStatusMetrics["temp"] = 100;
  serializeJson(jsonStatus, statusString);
}

SpaControl* SpaStatus::findByName(const char* name) {
  for (SpaControl* control: controls) {
    if (strcmp(control->name, name) == 0) {
      return control;
    }
  }
  Serial.println("No control found with name: ");
  Serial.println(name);
  throw std::invalid_argument("Control not found with this name");
}

void SpaStatus::applyControls() {
  for (SpaControl* control: controls) {
    control->applyOutputs();
  }
}
