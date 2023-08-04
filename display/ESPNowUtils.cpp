/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/?s=esp-now
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based on JC Servaye example: https://github.com/Servayejc/esp_now_web_server/
*/

#include "ESPNowUtils.h"

// ---------------------------- esp_ now -------------------------
void ESPNowUtils::printMAC(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
}

void ESPNowUtils::addPeer(const uint8_t * mac_addr, uint8_t chan){
    esp_now_peer_info_t peer;
    ESP_ERROR_CHECK(esp_wifi_set_channel(chan ,WIFI_SECOND_CHAN_NONE));
    esp_now_del_peer(mac_addr);
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = chan;
    peer.encrypt = false;
    memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
    if (esp_now_add_peer(&peer) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }
    memcpy(serverAddress, mac_addr, sizeof(uint8_t[6]));
}

void ESPNowUtils::sendOverrideCommand(u_int8_t control_id, time_t start, time_t end, u_int8_t value) {
    outgoingCommand.msgType = COMMAND;
    outgoingCommand.board_id = BOARD_ID;
    outgoingCommand.control_id = control_id;
    outgoingCommand.start = start;
    outgoingCommand.end = end;
    outgoingCommand.value = value;
    esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &outgoingCommand, sizeof(outgoingCommand));
}

void ESPNowUtils::registerDataCallbackControlHandler(hot_tub_control_status_recv_callback callbackFunc) {
    dataCallbackControl = callbackFunc;
}
void ESPNowUtils::registerDataCallbackServerHandler(hot_tub_server_status_recv_callback callbackFunc) {
    dataCallbackServer = callbackFunc;
}

void ESPNowUtils::registerEspCommStatusCallBackHandler(ESPNowUtils::esp_comm_status_callback callbackFunc) {
    espCommCallback = callbackFunc;
}

// callback when data is sent
void ESPNowUtils::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void ESPNowUtils::OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    Serial.print("Packet received from: ");
    printMAC(mac_addr);
    Serial.println();
    Serial.print("data size = ");
    Serial.println(sizeof(incomingData));
    uint8_t type = incomingData[0];
    lastMessageReceivedTime = millis();
    switch (type) {
        case CONTROL_STATUS :      // we received data from server
            memcpy(&receivedControlStatus, incomingData, sizeof(receivedControlStatus));
            Serial.print("ID  = ");
            Serial.print(receivedControlStatus.board_id);
            Serial.print(" control = ");
            Serial.print(receivedControlStatus.control_id);
            Serial.print(" value = ");
            Serial.println(receivedControlStatus.value);
            dataCallbackControl(&receivedControlStatus);
            break;
        case SERVER_STATUS:
            memcpy(&receivedServerStatus, incomingData, sizeof(receivedServerStatus));
            Serial.print("Received server status from ");
            Serial.println(receivedServerStatus.server_name);
            dataCallbackServer(&receivedServerStatus);
            break;
        case PAIRING:    // we received pairing data from server
            memcpy(&pairingData, incomingData, sizeof(pairingData));
            if (pairingData.board_id == 0) {              // the message comes from server
                printMAC(mac_addr);
                Serial.print("Pairing done for ");
                printMAC(pairingData.macAddr);
                Serial.print(" on channel " );
                Serial.print(pairingData.channel);    // channel used by the server
                Serial.print(" in ");
                Serial.print(millis()-pairingStartTime);
                Serial.println("ms");
                addPeer(pairingData.macAddr, pairingData.channel); // add the server  to the peer list
#ifdef SAVE_CHANNEL
                lastChannel = pairingData.channel;
                EEPROM.write(0, pairingData.channel);
                EEPROM.commit();
#endif
                pairingStatus = PAIR_PAIRED;             // set the pairing status
            }
            break;
    }
}


PairingStatus ESPNowUtils::autoPairing(){
    switch(pairingStatus) {
        case PAIR_REQUEST:
            Serial.print("Pairing request on channel "  );
            Serial.println(channel);

            // set WiFi channel
            ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
            if (esp_now_init() != ESP_OK) {
                Serial.println("Error initializing ESP-NOW");
            }

            // set callback routines
            esp_now_register_send_cb(OnDataSent);
            esp_now_register_recv_cb(OnDataRecv);

            // set pairing data to send to the server
            pairingData.msgType = PAIRING;
            pairingData.board_id = BOARD_ID;
            pairingData.channel = channel;

            // add peer and send request
            addPeer(serverAddress, channel);
            esp_now_send(serverAddress, (uint8_t *) &pairingData, sizeof(pairingData));
            previousMillis = millis();
            pairingStatus = PAIR_REQUESTED;

            break;

        case PAIR_REQUESTED:
            // time out to allow receiving response from server
            currentMillis = millis();
            if(currentMillis - previousMillis > 250) {
                previousMillis = currentMillis;
                // time out expired,  try next channel
                channel ++;
                if (channel > MAX_CHANNEL){
                    channel = 1;
                }
                pairingStatus = PAIR_REQUEST;
            }
            break;

        case PAIR_PAIRED:
            // nothing to do here
            break;
    }
    return pairingStatus;
}

void ESPNowUtils::setup() {
    previousMillis = 0;
    pairingStatus = NOT_PAIRED;

//    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
//    ESP_ERROR_CHECK(esp_wifi_start());
//    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    Serial.println();
    Serial.print("Client Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.print("Setting to WIFI_STA:  ");
    WiFi.mode(WIFI_STA);
    Serial.print("now, client Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    espCommCallback("Our MAC: %s", WiFi.macAddress().c_str());
    Serial.println("Printed mac");
    Serial.println("Wifi Mode set");
    WiFi.disconnect();
    delay(100); // may cure wifi not init error in set channel
    Serial.println("Wifi Done");
    pairingStartTime = millis();

#ifdef SAVE_CHANNEL
    EEPROM.begin(10);
    lastChannel = EEPROM.read(0);
    Serial.println(lastChannel);
    if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL) {
      channel = lastChannel;
    }
#endif
    pairingStatus = PAIR_REQUEST;
    Serial.println("ESPNowUtils Setup done");
    delay(500);
}

void ESPNowUtils::loop() {
    if (autoPairing() == PAIR_PAIRED) {
        if (lastMessageReceivedTime + MESSAGE_RECEIVED_MAX_AGE < millis()) {
            pairingStatus = PAIR_REQUEST; // re-start pairing if server restarts
        }
//    Serial.println(channel);
//        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= 10000) {
            // Save the last time a new reading was published
            previousMillis = currentMillis;
            espCommCallback("Determined WiFi Channel: %d", channel);
//            //Set values to send
//            myData.msgType = COMMAND;
//            myData.board_id = BOARD_ID;
//            myData.control_id = random(0,4);
//            myData.start = 0;
//            myData.end = 23;
//            myData.value = random(0,2);
//            esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &myData, sizeof(myData));
        }
    }
}

