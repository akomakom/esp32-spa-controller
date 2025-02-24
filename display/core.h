#ifndef HOT_TUB_DISPLAY_CORE_H
#define HOT_TUB_DISPLAY_CORE_H


#include "esp_system.h"

//Converts reason type to a C string.
//Type is located in /tools/sdk/esp32/include/esp_system/include/esp_system.h
const char *resetReasonName(esp_reset_reason_t r);


#endif //HOT_TUB_DISPLAY_CORE_H