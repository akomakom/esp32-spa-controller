#include "core.h"
//
// Created by akom on 8/4/23.
//
extern const char * PREFERENCES_NAME = "hot-tub";
extern Preferences app_preferences = Preferences();
extern struct tm * main_device_time = (tm*) malloc(sizeof(tm));
extern int timezone_offset = 0;