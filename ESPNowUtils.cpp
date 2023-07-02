/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/?s=esp-now
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based on JC Servaye example: https://github.com/Servayejc/esp_now_web_server/
*/

#include "ESPNowUtils.h"

void ESPNowUtils::readDataToSend() {
    outgoingSetpoints.msgType = CONTROL_STATUS;
    outgoingSetpoints.board_id = 0;
    outgoingSetpoints.value=222;
//    outgoingSetpoints.temp = random(0, 40);
//    outgoingSetpoints.hum = random(0, 100);
//    outgoingSetpoints.readingId = counter++;
}


// ---------------------------- esp_ now -------------------------
void ESPNowUtils::printMAC(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
}

bool ESPNowUtils::addPeer(const uint8_t *peer_addr) {      // add pairing
    memset(&slave, 0, sizeof(slave));
    const esp_now_peer_info_t *peer = &slave;
    memcpy(slave.peer_addr, peer_addr, 6);

    slave.channel = chan; // pick a channel
    slave.encrypt = 0; // no encryption
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if (exists) {
        // Slave already paired.
        Serial.println("Already Paired");
        return true;
    }
    else {
        esp_err_t addStatus = esp_now_add_peer(peer);
        if (addStatus == ESP_OK) {
            // Pair success
            Serial.println("Pair success");
            return true;
        }
        else
        {
            Serial.println("Pair failed");
            return false;
        }
    }
}

// callback when data is sent
void ESPNowUtils::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Last Packet Send Status: ");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
    printMAC(mac_addr);
    Serial.println();
}

void ESPNowUtils::OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    Serial.print(len);
    Serial.print(" bytes of data received from : ");
    printMAC(mac_addr);
    Serial.println();
    StaticJsonDocument<1000> root;
    uint8_t type = incomingData[0];       // first message byte is the type of message
    switch (type) {
        case COMMAND :                           // the message is data type
            memcpy(&incomingCommand, incomingData, sizeof(incomingCommand));
            callback(&incomingCommand);
//            // create a JSON document with received data and send it by event to the web page
//            root["id"] = incomingReadings.board_id;
//            root["temperature"] = incomingReadings.temp;
//            root["humidity"] = incomingReadings.hum;
//            root["readingId"] = String(incomingReadings.readingId);
//            Serial.print("event send :");
//            serializeJson(root, Serial);
//            Serial.println();
            break;

        case PAIRING:                            // the message is a pairing request
            memcpy(&pairingData, incomingData, sizeof(pairingData));
            Serial.println(pairingData.msgType);
            Serial.println(pairingData.board_id);
            Serial.print("Pairing request from: ");
            printMAC(mac_addr);
            Serial.println();
            Serial.println(pairingData.channel);
            if (pairingData.board_id > 0) {     // do not replay to server itself
                if (pairingData.msgType == PAIRING) {
                    pairingData.board_id = 0;       // 0 is server
                    // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
                    WiFi.softAPmacAddress(pairingData.macAddr);
                    pairingData.channel = chan;
                    Serial.println("send response");
                    esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &pairingData, sizeof(pairingData));
                    addPeer(mac_addr);
                }
            }
            break;
    }
}

void ESPNowUtils::registerDataCallBackHandler(hot_tub_command_recv_callback callbackFunc) {
    callback = callbackFunc;
}

void ESPNowUtils::initESP_NOW() {
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

void ESPNowUtils::setup() {
    Serial.print("Server SOFT AP MAC Address:  ");
    Serial.println(WiFi.softAPmacAddress());

    chan = WiFi.channel();
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());

    initESP_NOW();
}

void ESPNowUtils::loop() {
    static unsigned long lastEventTime = millis();
    static const unsigned long EVENT_INTERVAL_MS = 5000;
    if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
        lastEventTime = millis();
        readDataToSend();
        esp_now_send(NULL, (uint8_t *) &outgoingSetpoints, sizeof(outgoingSetpoints));
    }
}