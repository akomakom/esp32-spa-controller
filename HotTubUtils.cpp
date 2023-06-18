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

void SpaControl::init(const char* t_name, u_int8_t t_min, u_int8_t t_max) {
  name = t_name;
  min = t_min;
  max = t_max;
}

void SpaControl::toggle() {}
void SpaControl::applyOutputs() {}


/*** SimpleSpaControl ***/

SimpleSpaControl::SimpleSpaControl(const char* name, u_int8_t pin) : SpaControl(name) {
  pinMode(pin, OUTPUT);
}

SimpleSpaControl::SimpleSpaControl(const char* name, u_int8_t pin, u_int8_t min, u_int8_t max) : SpaControl(name, min, max) {
  pinMode(pin, OUTPUT);
}

void SimpleSpaControl::toggle() {
  value++;
  if (value > max) {
    value = 0;
  }
}

void SimpleSpaControl::applyOutputs() {
  digitalWrite(pin, value ? HIGH : LOW);
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
