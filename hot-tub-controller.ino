#include <WiFi.h>
#include <WiFiClient.h>
//#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "web.h"
#include "HotTubUtils.h"


SpaStatus spaStatus;

void setup(void) {
  Serial.begin(115200);
  // while(!Serial);

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
  Serial.println("mDNS responder started");

  server.on("/status", HTTP_GET, []() {
    spaStatus.updateStatusString();
    sendJSONResponse(spaStatus.statusString);
  });

  server.on("/toggle", HTTP_POST, []() {
    const char* response = WEB_RESPONSE_OK;
    Serial.print("Control passed is "); Serial.print(server.arg("control"));
    
    try {
      spaStatus.findByName(server.arg("control").c_str())->toggle();
      // Serial.print("With pin");
      // Serial.println(spaStatus.findByName(server.arg("control").c_str())->pin);
      sendJSONResponse(WEB_RESPONSE_OK);
    } catch (std::invalid_argument& e) {
      sendJSONResponse(WEB_RESPONSE_FAIL, 500);
    }
  });

  server.on("/", HTTP_GET, []() {
    sendHTMLResponse(WEBPAGE_CONTROLS);
  });

  setupServerDefaultActions();
  server.begin();
}

void loop(void) {
  server.handleClient();
  spaStatus.applyControls();
  yield();
}
