//
// Created by akom on 7/10/23.
//

#ifndef HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H
#define HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H

// Used only by client:
enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};

//TODO: affects json and esp-now
//enum ControlType {OFF_ON, OFF_LOW_HIGH, SENSOR_BASED};

enum MessageType {PAIRING, COMMAND, CONTROL_STATUS, SERVER_STATUS, METRICS_STATUS};
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

typedef struct struct_status_server {
    uint8_t msgType = SERVER_STATUS;
    uint8_t board_id;
    time_t time;
    float water_temp;
    int tz_offset = 0;
    char server_name[20] = "Hot Tub";
    u_int8_t control_count = 0;
    u_int16_t touchscreen_timeout = 0; // 0 is never
} struct_status_server;

typedef struct struct_status_control {
    uint8_t msgType = CONTROL_STATUS;
    uint8_t board_id;
    u_int8_t control_id;
    u_int8_t min;
    u_int8_t max;
    char type[15] = ""; // control type
    char name[15] = "";
    u_int32_t DO;   // Default override time
    u_int32_t ORT;  // override remaining time
    u_int32_t EL;  // override elapsed time
    u_int8_t value;
    u_int8_t e_value;
} struct_status_control;

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t board_id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;



#endif //HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H
