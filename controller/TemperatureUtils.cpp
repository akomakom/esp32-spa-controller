#include "TemperatureUtils.h"


float TemperatureUtils::getTempFByID(u_int8_t id)
{
    for (SensorAddressMapping *mapping: map) {
        if (mapping->id == id)
        {
            return sensors->getTempF(mapping->addr);
        }
    }
    return -999;
}


void TemperatureUtils::printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // zero pad the address if necessary
        if (deviceAddress[i] < 16) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

void TemperatureUtils::readTemperatures() {
    Serial.print("Requesting temperatures...");
    sensors->requestTemperatures();
    Serial.print(" ... ");

    //init cache
    for (u_int8_t i = 0 ; i < sensors->getDeviceCount(); i++) {
        float temp = sensors->getTempFByIndex(i);
        if (temp < MINIMUM_VALID_TEMP_F || temp > MAXIMUM_VALID_TEMP_F) {
            Serial.print("Invalid temperature reading: ");
            Serial.println(temp);
        } else {
            temperatureCache[i] = temp;
        }
        Serial.println(temperatureCache[i]);
    }
}

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

    // temporary:
    temperatureCache.push_back(0); //put in an initial value for first sensor
    temperatureCache.assign(sensors->getDeviceCount(), 0);  // and others as well, if any
    readTemperatures(); // ensure we have some data




    /***** This is not yet used *****/
    /**
     * TODO: dynamic mapping of DeviceAddress to role with a UI to configure
     */
    DeviceAddress waterSensorAddress;
    Serial.print("Water temperature sensor address is ");
    if (app_preferences.getBytes(KEY_TEMP_WATER_ADDRESS, &waterSensorAddress, sizeof(waterSensorAddress)) > 0) {
        // we already assigned a water temp sensor
        printAddress(waterSensorAddress);
        Serial.print(" (saved in EEPROM) ");
     }else {
        // let's pick the first one, if one exists
        if (sensors->getAddress(waterSensorAddress, 0)) {
            Serial.print("(Assigning this temperature sensor as water sensor... ) ");
//            Serial.print("sizeof is "); Serial.print(sizeof(waterSensorAddress));
            printAddress(waterSensorAddress);
            if (app_preferences.putBytes(KEY_TEMP_WATER_ADDRESS, &waterSensorAddress, sizeof(waterSensorAddress)) > 0) {
                Serial.print(" done! ");
            } else {
                Serial.print("Unable to save preferences... ");
            }
        } else {
            Serial.println("Unable to assign water temp sensor, perhaps none are present");
        }
    }
    Serial.println("");


//    // store IDs in DS18B20 user data EEPROM registers (first time only)
//    for (u_int8_t i = 0 ; i < sensors->getDeviceCount(); i++) {
//
//        int16_t data = sensors->getUserDataByIndex(i);
//        Serial.print("Sensor "); Serial.print(i); Serial.print(" has data "); Serial.println(data);
//        //sensors->setUserDataByIndex
//
//        SensorAddressMapping *mapping = new SensorAddressMapping();
//        map.push_back(mapping);
//
//        sensors->getAddress(mapping->addr, i);
//        mapping->id = sensors->getUserData(mapping->addr);
//        Serial.print("Sensor "); Serial.print(i); Serial.print(" has data "); Serial.println(mapping->id);
//        printAddress(mapping->addr);
//
//    }

}

void TemperatureUtils::loop() {
    if (now() > (temperatureRequestedTime + TEMPERATURE_REQUEST_FREQUENCY)) {
        temperatureRequestedTime = now();
        readTemperatures();
//        DeviceAddress device0;
//        Serial.print("getting address...");
//        sensors->getAddress(device0, 0);
//        Serial.print("temp by adress...");
//        Serial.println(sensors->getTempC(device0));
    }
}

//float TemperatureUtils::getTempC(u_int8_t sensorIndex) {
//    return sensors->getTempCByIndex(sensorIndex);
//}
float TemperatureUtils::getTempF(u_int8_t sensorIndex) {
    return temperatureCache[sensorIndex];
}