//
// Created by akom on 7/10/23.
//

#ifndef HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H
#define HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H

enum MessageType {PAIRING, COMMAND, CONTROL_STATUS, METRICS_STATUS};
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

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t board_id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;



#endif //HOT_TUB_CONTROLLER_HOT_TUB_TYPES_H
