#include <WiFi.h>
#include <WiFiClient.h>
//#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "web.h"
#include "HotTubUtils.h"
#include "ESPNowUtils.h"

// Interval at which to publish sensor readings
#define ESP_STATUS_SEND_INTERVAL  10000


SpaStatus spaStatus;
// Stores last time status was published on esp-now
unsigned long previousStatusSendTime = 0;

void setup(void) {
    Serial.begin(115200);

    if (!app_preferences.begin("hot-tub")) {
        Serial.println("Unable to open preferences");
    }

    // TODO: is timelib and ntpclient completely independent?
    // setenv("TZ","EST5EDT,M3.2.0,M11.1.0",1);

    // To support ESP-NOW
    // Set the device as a Station and Soft Access Point simultaneously
    WiFi.mode(WIFI_AP_STA);
    // Connect to WiFi network
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    Serial.println("");

    Serial.print("Connected to ");
    Serial.print(WIFI_NAME);
    Serial.print(" IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println("");

//    /*use mdns for host name resolution*/
//    if (!MDNS.begin(WIFI_HOST)) { //http://esp32.local
//        Serial.println("Error setting up MDNS responder!");
//        while (1) {
//            delay(1000);
//        }
//    }

    SPIFFS.begin();
    Serial.println("mDNS responder started");

    server.on("/status", HTTP_GET, []() {
        spaStatus.updateStatusString();
        sendJSONResponse(spaStatus.statusString);
    });

    server.on("/toggle", HTTP_POST, []() {
        const char *response = WEB_RESPONSE_OK;
        try {
            spaStatus.findByName(server.arg("control").c_str())->toggle();
            previousStatusSendTime = 0; // update all others
            sendJSONResponse(WEB_RESPONSE_OK);
        } catch (std::invalid_argument &e) {
            sendJSONResponse(e.what(), 500);
        }
    });
    server.on("/override", HTTP_POST, []() {
        const char *response = WEB_RESPONSE_OK;
        try {
            // seconds relative to current time
            time_t start = now() + (time_t)server.arg("start").toInt();
            time_t end   = now() + (time_t)server.arg("duration").toInt();
            u_int8_t value = (u_int8_t)server.arg("value").toInt();
            spaStatus.findByName(server.arg("control").c_str())->scheduleOverride(start, end, value);
            previousStatusSendTime = 0; //update all others
            sendJSONResponse(WEB_RESPONSE_OK);
        } catch (std::invalid_argument &e) {
            sendJSONResponse(e.what(), 500);
        }
    });
    server.on("/configureControl", HTTP_GET, []() {
        SpaControl *control = spaStatus.findByName(server.arg("control").c_str());
        control->updateConfigJsonString();
        sendJSONResponse(control->configString);
    });
    server.on("/configureControl", HTTP_POST, []() {
        try {
            SpaControl *control = spaStatus.findByName(server.arg("control").c_str());
            control->normalSchedule(
                    (u_int8_t)server.arg("percentageOfDayOnTime").toInt(),
                    (u_int8_t)server.arg("numberOfTimesToRun").toInt(),
                    (u_int8_t)server.arg("normalValueOn").toInt(),
                    (u_int8_t)server.arg("normalValueOff").toInt()
            );
            control->normalSettings.overrideDefaultDurationSeconds = (u_int32_t)server.arg("overrideDefaultDurationSeconds").toInt();
            control->persist();
            previousStatusSendTime = 0; // update others
            sendJSONResponse(WEB_RESPONSE_OK);
        } catch (std::invalid_argument &e) {
            sendJSONResponse(e.what(), 500);
        }
    });

    setupServerDefaultActions();
    server.begin();

    spaStatus.setup();
    ESPNowUtils::setup();
    ESPNowUtils::registerDataCallBackHandler((ESPNowUtils::hot_tub_command_recv_callback)espnowCommandReceived);

}

void loop(void) {
    server.handleClient();
    spaStatus.loop();
    ESPNowUtils::loop();
    sendStatus();
    yield();
//    sleep(5);
}

void sendStatus() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousStatusSendTime >= ESP_STATUS_SEND_INTERVAL) {
        // Save the last time a new reading was published
        previousStatusSendTime = currentMillis;

        ESPNowUtils::outgoingStatusServer.time = timeClient->getEpochTime();
        // TODO:
//        ESPNowUtils::outgoingStatusServer.server_name =

        ESPNowUtils::sendStatusServer();

        for (int i = 0 ; i < spaStatus.controls.size() ; i++) {
            SpaControl* control = spaStatus.controls[i];
            ESPNowUtils::outgoingStatusControl.control_id = i;
            ESPNowUtils::outgoingStatusControl.min = control->min;
            ESPNowUtils::outgoingStatusControl.max = control->max;
            strcpy(ESPNowUtils::outgoingStatusControl.type, control->type);
            strcpy(ESPNowUtils::outgoingStatusControl.name, control->name);
            ESPNowUtils::outgoingStatusControl.ORT = control->getOverrideScheduleRemainingTime();
            ESPNowUtils::outgoingStatusControl.DO = control->normalSettings.overrideDefaultDurationSeconds;
            ESPNowUtils::outgoingStatusControl.value = control->getEffectiveValue();
            ESPNowUtils::outgoingStatusControl.e_value = control->getEffectiveValueForDependents();

            ESPNowUtils::sendStatusControl();
        }
    }
}

/**
 * Handle incoming commands received via ESP-NOW
 * @param command
 */
void espnowCommandReceived(struct_command *command) {
    Serial.print("Received command: control=");Serial.print(command->control_id);
    Serial.print(" value=");Serial.print(command->value);
    Serial.print(" end=");Serial.print(command->end);
    Serial.println();

    previousStatusSendTime = 0; // update others
    spaStatus.controls[command->control_id]->scheduleOverride(
            now() + command->start,
            now() + command->end,
            command->value
    );

}