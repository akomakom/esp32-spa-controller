//
// Created by akom on 7/1/23.
//

#ifndef HOT_TUB_CONTROLLER_ESPNOWUTILS_H
#define HOT_TUB_CONTROLLER_ESPNOWUTILS_H

#include <esp_now.h>
#include <WiFi.h>
//#include "AsyncTCP.h"
#include <ArduinoJson.h>

#include "hot_tub_types.h"


class ESPNowUtils {
public:

    typedef void (*hot_tub_command_recv_callback)(struct_command *command);
    typedef void (*hot_tub_paired_callback)(struct_pairing *pairing);

    static void setup();
    static void loop();
    static void registerCommandCallBackHandler(hot_tub_command_recv_callback callbackFunc);
    static void registerPairingCallBackHandler(hot_tub_paired_callback callbackFunc);
    // relies on someone updating outgoingStatusControl first
    static void sendStatusControl();
    static void sendStatusServer();

    inline static struct_status_server  outgoingStatusServer;
    inline static struct_status_control outgoingStatusControl;
private:
    inline static esp_now_peer_info_t slave;
    inline static int chan;

    static MessageType messageType;

    inline static int counter = 0;


    inline static struct_command incomingCommand;
    inline static struct_pairing pairingData;
    inline static hot_tub_command_recv_callback commandCallback = NULL;
    inline static hot_tub_paired_callback pairedCallback = NULL;


    static void readDataToSend();
    static void printMAC(const uint8_t * mac_addr);
    static bool addPeer(const uint8_t *peer_addr);
    static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void OnDataRecv(const esp_now_recv_info* info, const uint8_t *incomingData, int len);
    static void initESP_NOW();
};

#endif //HOT_TUB_CONTROLLER_ESPNOWUTILS_H
