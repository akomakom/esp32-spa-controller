//
// Created by akom on 7/1/23.
//

#ifndef HOT_TUB_CONTROLLER_ESPNOWUTILS_H
#define HOT_TUB_CONTROLLER_ESPNOWUTILS_H

#include <esp_now.h>
#include <WiFi.h>
//#include "AsyncTCP.h"
#include <ArduinoJson.h>
#include <TimeLib.h>

class ESPNowUtils {
public:
    // Structure to receive data
    // Must match the sender structure
    typedef struct struct_command {
        uint8_t msgType;
        uint8_t board_id;
        u_int8_t control_id;
        time_t start;
        time_t end;
        u_int8_t value;
    } struct_command;

    typedef struct struct_status_control {
        uint8_t msgType;
        uint8_t board_id;
        u_int8_t control_id;
        u_int8_t min;
        u_int8_t max;
        const char* type;
        const char* name;
        long ort;
        u_int8_t value;
    } struct_status_control;

    typedef void (*hot_tub_command_recv_callback)(struct_command *command);

    static void setup();
    static void loop();
    static void registerDataCallBackHandler(hot_tub_command_recv_callback callbackFunc);
    // relies on someone updating outgoingStatusControl first
    static void sendStatusControl();

    inline static struct_status_control outgoingStatusControl;
private:
    inline static esp_now_peer_info_t slave;
    inline static int chan;

    enum MessageType {PAIRING, COMMAND, CONTROL_STATUS, METRICS_STATUS};
    static MessageType messageType;

    inline static int counter = 0;


    typedef struct struct_pairing {       // new structure for pairing
        uint8_t msgType;
        uint8_t board_id;
        uint8_t macAddr[6];
        uint8_t channel;
    } struct_pairing;

    inline static struct_command incomingCommand;
    inline static struct_pairing pairingData;
    inline static hot_tub_command_recv_callback callback;


    static void readDataToSend();
    static void printMAC(const uint8_t * mac_addr);
    static bool addPeer(const uint8_t *peer_addr);
    static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len);
    static void initESP_NOW();
};

#endif //HOT_TUB_CONTROLLER_ESPNOWUTILS_H
