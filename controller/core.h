//
// Created by akom on 7/18/23.
//

#ifndef HOT_TUB_CONTROLLER_CORE_H
#define HOT_TUB_CONTROLLER_CORE_H

#include <Preferences.h>

// NTP stuff
#include "time.h"
#include "sntp.h"

extern const char * PREFERENCES_NAME;
extern Preferences app_preferences;
extern struct tm *main_device_time;
extern int timezone_offset;  // inferred at runtime, this is not a configuration parameter.
// Winterized mode: when true, every control is forced OFF and schedules/overrides are
// ignored (stored settings are preserved). Persisted in NVS ("winterized"); cached here
// and loaded at boot. See getOnState() in HotTubUtils.cpp.
extern bool spaWinterized;
//
//typedef struct spa_config {
//
//} spa_config;

#endif //HOT_TUB_CONTROLLER_CORE_H
