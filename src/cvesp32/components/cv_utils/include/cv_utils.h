#pragma once
#include "esp_system.h"

#define WIFI_CRED_MAXLEN 32
extern char desired_ap_ssid[WIFI_CRED_MAXLEN];
extern char desired_ap_pass[WIFI_CRED_MAXLEN];
extern char desired_friendly_name[WIFI_CRED_MAXLEN];
extern char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];
extern int node_number;

extern void get_chip_id(char* ssid, const int UNIQUE_ID_LENGTH);
extern void remove_ctrlchars(char *str);
extern bool set_credential(char* credentialName, char* val);
extern bool start_nvs();
extern bool read_nvs_value(void);