#pragma once
#include "esp_system.h"

typedef enum  {
    CVWIFI_M_NOT_STARTED,
    CVWIFI_OFF,
    CVWIFI_STA,
    CVWIFI_AP
} CV_WIFI_MODE;


typedef enum  {
    CVWIFI_NO_CHANGE, // Returned if cv_wifi is requested to change state to a state it is already in
    CVWIFI_SUCCESS, // Returned if state change successful
    CVWIFI_DISABLED, // Returned if the requested wifi mode is disabled
    CVWIFI_NOT_STARTED, // Returned if the wifi state machine hasn't been started
    CVWIFI_STATE_ERROR // Unknown state error
} CV_WIFI_STATE_MSG;

extern void start_wifi(char* PARAM_ESP_WIFI_SSID, uint8_t PARAM_SSID_LEN);


extern CV_WIFI_MODE get_wifi_mode();
extern CV_WIFI_STATE_MSG set_wifi_mode(CV_WIFI_MODE mode);

extern uint8_t get_wifi_power();
extern void set_wifi_power(int8_t power);
extern bool set_wifi_power_pChr(char* power);


