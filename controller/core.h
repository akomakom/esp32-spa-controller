//
// Created by akom on 7/18/23.
//

#ifndef HOT_TUB_CONTROLLER_CORE_H
#define HOT_TUB_CONTROLLER_CORE_H

#include <Preferences.h>

// NTP stuff
#include <NTPClient.h>
#include <WiFiUdp.h>

extern Preferences app_preferences;
extern NTPClient *timeClient;

#endif //HOT_TUB_CONTROLLER_CORE_H
