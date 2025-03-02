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
#define ESP_STATUS_SEND_INTERVAL  1000

#define NTP_SYNC_INTERVAL 3600 * 1000
// If we don't get NTP sync success for this long, reboot (assume that network failed)
#define NTP_SYNC_FAIL_REBOOT_INTERVAL NTP_SYNC_INTERVAL * 24

// TimeZone rule including daylight adjustment rules (optional)
// TODO: move to Preferences
const char* time_zone = "EST5EDT,M3.2.0,M11.1.0";
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
unsigned long  ntp_last_sync_success_time = 0;

SpaStatus spaStatus;
// Stores last time status was published on esp-now
unsigned long previousStatusSendTime = 0;

void setDeviceTime(){
    static unsigned long lastPrint;
    bool debug = ((lastPrint + 10000) < millis());
    if (debug) {
        lastPrint = millis();
    }

    if(!getLocalTime(main_device_time, 9)){
        if ((lastPrint + 10000) < millis()) {
            Serial.println("No time available (yet)");
            lastPrint = millis();
        }
        return;
    }
    if (debug) {
        Serial.println(main_device_time, "Determined time: %A, %B %d %Y %H:%M:%S");
    }
//    static time_t t = mktime(main_device_time);
//    struct timeval now = { .tv_sec = t };
    // this seems to be causing time to stand still:
//    settimeofday(&now, NULL); // is this even useful?

    // calculate timezone offset

    static struct tm  linfo, ginfo;
    static time_t  rawt;
    time( &rawt );
    gmtime_r( &rawt, &ginfo);
    localtime_r( &rawt, &linfo);

    time_t g = mktime(&ginfo);
    time_t l = mktime(&linfo);

    static int    offsetSeconds = difftime( l, g );
    static int    offsetHours   = (int)offsetSeconds /(60*60);
    // for our globals, we care about actual time offset (including DST)
    timezone_offset = linfo.tm_isdst ? offsetSeconds + 3600 : offsetSeconds;
    // print out debugs every minute
    if ((lastPrint + 60000) < millis()) {
//        Serial.printf("Setting time to %s\n", asctime(main_device_time));
        Serial.printf("Timezone offset is %d hour%s (%d seconds), %s  current offset is %d hour%s (%d seconds)\n\n",
                      offsetHours,
                      (offsetHours > 1 || offsetHours < -1) ? "s" : "",
                      offsetSeconds,
                      linfo.tm_isdst ? "DST in effect" : "DST not in effect",
                      linfo.tm_isdst ? offsetHours + 1 : offsetHours,
                      ((offsetHours + 1) > 1 || (offsetHours + 1) < -1) ? "s" : "",
                      timezone_offset
                      );

//        Serial.printf("Device time: %d\n", rawt);
        lastPrint = millis();
    }
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    setDeviceTime();
    ntp_last_sync_success_time = millis();
}

void setupNTP() {
    sntp_set_time_sync_notification_cb( timeavailable );
    sntp_set_sync_interval(NTP_SYNC_INTERVAL) ; // This is probably default, but I want to make sure it's set to an hour.
    /**
     * NTP server address could be aquired via DHCP,
     *
     * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
     * otherwise SNTP option 42 would be rejected by default.
     * NOTE: configTime() function call if made AFTER DHCP-client run
     * will OVERRIDE aquired NTP server address
     */
//    sntp_servermode_dhcp(1);    // (optional)

    /**
     * This will set configured ntp servers and constant TimeZone/daylightOffset
     * should be OK if your time zone does not need to adjust daylightOffset twice a year,
     * in such a case time adjustment won't be handled automagicaly.
     */
//    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    /**
     * A more convenient approach to handle TimeZones with daylightOffset
     * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
     * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
     */
    configTzTime(time_zone, ntpServer1, ntpServer2);
}

void setupWIFI() {

    // delete old config
    WiFi.disconnect(true);

    delay(1000);

    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    // To support ESP-NOW
    // Set the device as a Station and Soft Access Point simultaneously
    WiFi.mode(WIFI_AP_STA);
    // Connect to WiFi network
    if (app_preferences.isKey("wifi_ssid")) {
      WiFi.begin(app_preferences.getString("wifi_ssid"), app_preferences.getString("wifi_pass"));
      Serial.printf("WiFi begin called, connecting to %s", app_preferences.getString("wifi_ssid"));
    } else {
      Serial.println("WiFi not configured yet, not starting.");
    }
    delay(1000);
}

void setupSoftAP() {
    //
    WiFi.softAP(SOFTAP_WIFI_NAME, SOFTAP_WIFI_PASS);
    Serial.printf("Software Access Point available at %s/%s", SOFTAP_WIFI_NAME, SOFTAP_WIFI_PASS == NULL ? "(NOT SET)": SOFTAP_WIFI_PASS);
}

//
//void handleRequest()
//{
//  if (server.method() == HTTP_POST) {
//    strncpy(wiFiCredentials.ssid, server.arg("ssid").c_str(), sizeof(wiFiCredentials.ssid));
//    strncpy(wiFiCredentials.password, server.arg("password").c_str(), sizeof(wiFiCredentials.password));
//
//    wiFiCredentials.ssid[server.arg("ssid").length()] = wiFiCredentials.password[server.arg("password").length()] = 0;
//
//    EEPROM.put(0, wiFiCredentials);
//    EEPROM.commit();
//
//    const char * content = "<html><head> <link rel=\"stylesheet\" href=\"https://netdna.bootstrapcdn.com/bootstrap/3.0.2/css/bootstrap.min.css\"> <style>body{background: #eee !important;}.wrapper{margin-top: 80px; margin-bottom: 80px;}.form-signin, .result-message{max-width: 380px; padding: 15px 35px 45px; margin: 0 auto; background-color: #fff; border: 1px solid rgba(0, 0, 0, 0.1);}.form-signin .form-signin-heading{margin-bottom: 30px;}.form-signin .form-control{position: relative; font-size: 16px; height: auto; padding: 10px; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; box-sizing: border-box;}.form-signin .form-control:focus{z-index: 2;}.form-signin input[type=text]{margin-bottom: -1px; border-bottom-left-radius: 0; border-bottom-right-radius: 0;}.form-signin input[type=password]{margin-bottom: 20px; border-top-left-radius: 0; border-top-right-radius: 0;}.result-message{text-align: center;}</style></head><body> <div class=\"wrapper\"> <div class=\"result-message\"> The Wi-Fi credentials have been stored. <br/> Please reboot the device! </div></div></body></html> ";
//    server.send(200, "text/html", content);
//  } else {
//    const char * formHtml = "<html><head> <link rel=\"stylesheet\" href=\"https://netdna.bootstrapcdn.com/bootstrap/3.0.2/css/bootstrap.min.css\"> <style>body{background: #eee !important;}.wrapper{margin-top: 80px; margin-bottom: 80px;}.form-signin{max-width: 380px; padding: 15px 35px 45px; margin: 0 auto; background-color: #fff; border: 1px solid rgba(0, 0, 0, 0.1);}.form-signin .form-signin-heading{margin-bottom: 30px;}.form-signin .form-control{position: relative; font-size: 16px; height: auto; padding: 10px; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; box-sizing: border-box;}.form-signin .form-control:focus{z-index: 2;}.form-signin input[type=text]{margin-bottom: -1px; border-bottom-left-radius: 0; border-bottom-right-radius: 0;}.form-signin input[type=password]{margin-bottom: 20px; border-top-left-radius: 0; border-top-right-radius: 0;}</style></head><body> <div class=\"wrapper\"> <form class=\"form-signin\" method=\"POST\"> <h2 class=\"form-signin-heading\">Connect To Wi-Fi</h2> <input type=\"text\" class=\"form-control\" name=\"ssid\" placeholder=\"SSID\" required=\"\" autofocus=\"\"/> <input type=\"password\" class=\"form-control\" name=\"password\" placeholder=\"Password\" required=\"\"/> <button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">Save</button> </form> </div></body></html> ";
//    server.send(200, "text/html", formHtml);
//  }
//}

void setup(void) {
    Serial.begin(115200);

    if (!app_preferences.begin(PREFERENCES_NAME)) {
        Serial.println("Unable to open preferences");
    }

    setupNTP();
    setupSoftAP();
    setupWIFI();

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
    ESPNowUtils::registerCommandCallBackHandler((ESPNowUtils::hot_tub_command_recv_callback)espnowCommandReceived);
    ESPNowUtils::registerPairingCallBackHandler((ESPNowUtils::hot_tub_paired_callback)peerAdded);

}

void loop(void) {
    setDeviceTime();
    server.handleClient();
    spaStatus.loop();
    ESPNowUtils::loop();
    sendStatus();
    yield();
//    sleep(5);

    // reboot check
    if ((millis() - ntp_last_sync_success_time) > NTP_SYNC_FAIL_REBOOT_INTERVAL) {
        Serial.printf("We have not seen an NTP update in over %d, assuming network broke, rebooting", NTP_SYNC_FAIL_REBOOT_INTERVAL);
        ESP.restart();
    }
}

void peerAdded(struct_pairing pairing) {
    previousStatusSendTime = 0; // update others
}

void sendStatus() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousStatusSendTime >= ESP_STATUS_SEND_INTERVAL) {
        // Save the last time a new reading was published
        previousStatusSendTime = currentMillis;

        ESPNowUtils::outgoingStatusServer.time = mktime(main_device_time);
        ESPNowUtils::outgoingStatusServer.tz_offset = timezone_offset;
        // TODO: move this to a dynamic thing of some sort
        ESPNowUtils::outgoingStatusServer.water_temp = spaStatus.temperatureUtils.getTempF(0);
        ESPNowUtils::outgoingStatusServer.control_count = spaStatus.controls.size();
        ESPNowUtils::outgoingStatusServer.touchscreen_timeout = 300;

        // TODO:
//        ESPNowUtils::outgoingStatusServer.server_name =

        ESPNowUtils::sendStatusServer();

        for (int i = 0 ; i < spaStatus.controls.size() ; i++) {
            yield(); // this really slows down updates
            delay(1);
            SpaControl* control = spaStatus.controls[i];
            ESPNowUtils::outgoingStatusControl.control_id = i;
            ESPNowUtils::outgoingStatusControl.min = control->min;
            ESPNowUtils::outgoingStatusControl.max = control->max;
            strcpy(ESPNowUtils::outgoingStatusControl.type, control->type);
            strcpy(ESPNowUtils::outgoingStatusControl.name, control->name);
            ESPNowUtils::outgoingStatusControl.ORT = control->getOverrideScheduleRemainingTime();
            ESPNowUtils::outgoingStatusControl.DO = control->normalSettings.overrideDefaultDurationSeconds;
            ESPNowUtils::outgoingStatusControl.EL = control->getOverrideScheduleElapsedTime();

            ESPNowUtils::outgoingStatusControl.value = control->getEffectiveValue();
            ESPNowUtils::outgoingStatusControl.e_value = control->getOnState();

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


// WIFI:

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Connected to AP successfully!");
    Serial.println("");

    Serial.print("Connected to ");
    Serial.print(app_preferences.getString("wifi_ssid"));
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    WiFi.begin(app_preferences.getString("wifi_ssid"), app_preferences.getString("wifi_pass"));

}