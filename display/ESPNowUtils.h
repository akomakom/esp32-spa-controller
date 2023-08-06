//
// Created by akom on 7/1/23.
//

#ifndef HOT_TUB_CONTROLLER_ESPNOWUTILS_H
#define HOT_TUB_CONTROLLER_ESPNOWUTILS_H

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
//#include "AsyncTCP.h"
#include <EEPROM.h>

#include <TimeLib.h>

#include "hot_tub_types.h"

// Set your Board and Server ID
#define BOARD_ID 1
// for North America // 13 in Europe
#define MAX_CHANNEL 11
// millis, if no message in that time, re-pair
#define MESSAGE_RECEIVED_MAX_AGE 60000

class ESPNowUtils {
public:

    typedef void (*hot_tub_control_status_recv_callback)(struct_status_control *status);
    typedef void (*hot_tub_server_status_recv_callback)(struct_status_server *status);
    typedef void (*esp_comm_status_callback)(const char *messageFormat, ...);

    static void setup();
    static void loop();
    static void registerDataCallbackControlHandler(hot_tub_control_status_recv_callback callbackFunc);
    static void registerDataCallbackServerHandler(hot_tub_server_status_recv_callback callbackFunc);
    static void registerEspCommStatusCallBackHandler(esp_comm_status_callback callbackFunc);
    // relies on someone updating outgoingStatusControl first
    static void sendOverrideCommand(u_int8_t control_id, time_t start, time_t end, u_int8_t value);

    inline static struct_command outgoingCommand;
private:

    inline static int channel = MAX_CHANNEL; // start from last, CH 11 is most common
    inline static unsigned long lastMessageReceivedTime = 0;
    inline static uint8_t serverAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    inline static unsigned long previousMillis = 0;   // for pairing
    inline static unsigned long currentMillis = 0;
    inline static unsigned long pairingStartTime = 0; // used to measure Pairing time

    inline static hot_tub_control_status_recv_callback dataCallbackControl;
    inline static hot_tub_server_status_recv_callback dataCallbackServer;
    inline static esp_comm_status_callback espCommCallback;

    inline static struct_command myData;  // data to send
    inline static struct_status_control receivedControlStatus;  // data received
    inline static struct_status_server receivedServerStatus;  // data received
    inline static struct_pairing pairingData;
    inline static PairingStatus pairingStatus;
//    inline static int chan;
//
//    static MessageType messageType;
//
//    inline static int counter = 0;
//
//
//    inline static struct_command incomingCommand;
//    inline static struct_pairing pairingData;
//    inline static hot_tub_command_recv_callback callback;
    static void addPeer(const uint8_t * mac_addr, uint8_t chan);
    static void printMAC(const uint8_t * mac_addr);
    static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
    static PairingStatus autoPairing();
};

#endif //HOT_TUB_CONTROLLER_ESPNOWUTILS_H
