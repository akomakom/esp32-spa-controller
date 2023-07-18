#include "TemperatureUtils.h"


void TemperatureUtils::setup() {
    this->oneWire = new OneWire(ONE_WIRE_BUS);
    this->sensors = new DallasTemperature(oneWire);
    // Start up 1-wire stuff
    sensors->begin();
    // locate devices on the bus
    Serial.print("Locating sensors...");
    Serial.print("Found ");
    Serial.print(sensors->getDeviceCount(), DEC);
    Serial.println(" devices.");
}

void TemperatureUtils::loop() {
    if (now() > (temperatureRequestedTime + TEMPERATURE_REQUEST_FREQUENCY)) {
        temperatureRequestedTime = now();
        Serial.print("Requesting temperatures...");
        sensors->requestTemperatures();
        Serial.print(" ... ");
        Serial.println(sensors->getTempFByIndex(0));
//        DeviceAddress device0;
//        Serial.print("getting address...");
//        sensors->getAddress(device0, 0);
//        Serial.print("temp by adress...");
//        Serial.println(sensors->getTempC(device0));
    }
}

u_int8_t TemperatureUtils::getTempC(u_int8_t sensorIndex) {
    return sensors->getTempCByIndex(sensorIndex);
}
u_int8_t TemperatureUtils::getTempF(u_int8_t sensorIndex) {
    return sensors->getTempFByIndex(sensorIndex);
}