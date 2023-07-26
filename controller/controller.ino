#include <WiFi.h>
#include <WiFiClient.h>
//#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "web.h"
#include "HotTubUtils.h"
#include "ESPNowUtils.h"

SpaStatus spaStatus;
// Stores last time status was published on esp-now
unsigned long previousStatusSendTime = 0;
const long statusSendinterval = 10000;        // Interval at which to publish sensor readings

void setup(void) {
    Serial.begin(115200);

    if (!app_preferences.begin("hot-tub")) {
        Serial.println("Unable to open preferences");
    }
    // while(!Serial);

    // To support ESP-NOW
    // Set the device as a Station and Soft Access Point simultaneously
    WiFi.mode(WIFI_AP_STA);
    // Connect to WiFi network
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    Serial.println("");

    Serial.print("Connnecting to WiFi SSID: ");
    Serial.println(WIFI_NAME);
    // Wait for connection
    // TODO: probably a bad idea
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        Serial.print(".");
//    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_NAME);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

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
}

void sendStatus() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousStatusSendTime >= statusSendinterval) {
        // Save the last time a new reading was published
        previousStatusSendTime = currentMillis;

        for (int i = 0 ; i < spaStatus.controls.size() ; i++) {
            SpaControl* control = spaStatus.controls[i];
            ESPNowUtils::outgoingStatusControl.control_id = i;
            ESPNowUtils::outgoingStatusControl.min = control->min;
            ESPNowUtils::outgoingStatusControl.max = control->max;
            ESPNowUtils::outgoingStatusControl.type = control->type;
            strcpy(ESPNowUtils::outgoingStatusControl.name, control->name);
            ESPNowUtils::outgoingStatusControl.ort = control->getOverrideScheduleRemainingTime();
            ESPNowUtils::outgoingStatusControl.value = control->getEffectiveValue();

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
    spaStatus.controls[command->control_id]->scheduleOverride(
            now() + command->start,
            now() + command->end,
            command->value
    );

}