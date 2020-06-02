#pragma once
#include "esp_system.h"

#define WIFI_CRED_MAXLEN 32
extern char desired_ap_ssid[WIFI_CRED_MAXLEN];
extern char desired_ap_pass[WIFI_CRED_MAXLEN];
extern char desired_friendly_name[WIFI_CRED_MAXLEN];
extern char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];
extern uint8_t desired_node_number;

extern void get_chip_id(char* ssid, const int UNIQUE_ID_LENGTH);
extern void remove_ctrlchars(char *str);
extern bool set_credential(char* credentialName, char* val);
extern bool start_nvs();
static bool load_all_nvs(void);


typedef enum  {
    nvs_wifi_ssid,
    nvs_wifi_pass,
    nvs_fname,
    nvs_broker_ip,
    nvs_node_number
} CV_NVS_KEY;
extern bool get_nvs_value(CV_NVS_KEY nvs_key);
extern bool set_nvs_u8val(CV_NVS_KEY nvs_key, uint8_t val);
extern bool set_nvs_strval(CV_NVS_KEY nvs_key, char* val);