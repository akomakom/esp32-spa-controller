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

void setup(void) {
    Serial.begin(115200);
    // while(!Serial);

    // To support ESP-NOW
    // Set the device as a Station and Soft Access Point simultaneously
    WiFi.mode(WIFI_AP_STA);
    // Connect to WiFi network
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_NAME);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    /*use mdns for host name resolution*/
    if (!MDNS.begin(WIFI_HOST)) { //http://esp32.local
        Serial.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
        }
    }
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
            sendJSONResponse(WEB_RESPONSE_OK);
        } catch (std::invalid_argument &e) {
            sendJSONResponse(e.what(), 500);
        }
    });

    setupServerDefaultActions();
    server.begin();

    ESPNowUtils::setup();
    ESPNowUtils::registerDataCallBackHandler((ESPNowUtils::hot_tub_command_recv_callback)espnowCommandReceived);

}

void loop(void) {
    server.handleClient();
    spaStatus.applyControls();
    ESPNowUtils::loop();
    yield();
}

void espnowCommandReceived(ESPNowUtils::struct_command *command) {
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