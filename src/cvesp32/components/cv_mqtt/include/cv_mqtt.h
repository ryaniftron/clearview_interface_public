#pragma once
#include "esp_system.h"
extern bool update_subscriptions_new_seat();
extern void cv_mqtt_init(char* chipid, uint8_t chip_len, const char* mqtt_hostname); 
extern void mqtt_app_stop();