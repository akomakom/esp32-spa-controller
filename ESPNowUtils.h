//
// Created by akom on 7/1/23.
//

#ifndef HOT_TUB_CONTROLLER_ESPNOWUTILS_H
#define HOT_TUB_CONTROLLER_ESPNOWUTILS_H

#include <esp_now.h>
#include <WiFi.h>
//#include "AsyncTCP.h"
#include <ArduinoJson.h>

class ESPNowUtils {
public:
    static void setup();
    static void loop();
private:
    inline static esp_now_peer_info_t slave;
    inline static int chan;

    enum MessageType {PAIRING, DATA,};
    static MessageType messageType;

    inline static int counter = 0;

    // Structure example to receive data
    // Must match the sender structure
    typedef struct struct_message {
        uint8_t msgType;
        uint8_t id;
        float temp;
        float hum;
        unsigned int readingId;
    } struct_message;

    typedef struct struct_pairing {       // new structure for pairing
        uint8_t msgType;
        uint8_t id;
        uint8_t macAddr[6];
        uint8_t channel;
    } struct_pairing;

    inline static struct_message incomingReadings;
    inline static struct_message outgoingSetpoints;
    inline static struct_pairing pairingData;


    static void readDataToSend();
    static void printMAC(const uint8_t * mac_addr);
    static bool addPeer(const uint8_t *peer_addr);
    static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
    static void initESP_NOW();
};

#endif //HOT_TUB_CONTROLLER_ESPNOWUTILS_H
