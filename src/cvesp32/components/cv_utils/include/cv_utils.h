#pragma once
#include "esp_system.h"


#ifndef LOG_FMT_FUNCNAME
/* Formats a log string to prepend context function name */
// https://github.com/espressif/esp-idf/blob/47253a827a80cb78f22db8e4736c668804c16750/components/esp_http_server/src/esp_httpd_priv.h
#define LOG_FMT_FUNCNAME(x)      "%s: " x, __func__
#endif

// Use variadic Macros in C++14
// https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
//macros to log with function name
#define CV_LOGE(tag,fmt, ...)  ESP_LOGE (tag, LOG_FMT_FUNCNAME(fmt) __VA_OPT__(,) __VA_ARGS__) // error (lowest)
#define CV_LOGW(tag,fmt, ...)  ESP_LOGW (tag, LOG_FMT_FUNCNAME(fmt) __VA_OPT__(,) __VA_ARGS__) // warning
#define CV_LOGI(tag,fmt, ...)  ESP_LOGI (tag, LOG_FMT_FUNCNAME(fmt) __VA_OPT__(,) __VA_ARGS__) // info
#define CV_LOGD(tag,fmt, ...)  ESP_LOGD (tag, LOG_FMT_FUNCNAME(fmt) __VA_OPT__(,) __VA_ARGS__) // debug
#define CV_LOGV(tag,fmt, ...)  ESP_LOGV (tag, LOG_FMT_FUNCNAME(fmt) __VA_OPT__(,) __VA_ARGS__) // verbose (highest)


#define WIFI_CRED_MAXLEN 32
extern char desired_ap_ssid[WIFI_CRED_MAXLEN];
extern char desired_ap_pass[WIFI_CRED_MAXLEN];
extern char desired_friendly_name[WIFI_CRED_MAXLEN];
extern char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];
extern uint8_t desired_seat_number;

#define UNIQUE_ID_LENGTH 16
extern void get_chip_id(char* ssid, const int len);
extern void remove_ctrlchars(char *str);
extern bool set_credential(char* credentialName, char* val);
extern bool start_nvs();


typedef enum  {
    nvs_wifi_ssid,
    nvs_wifi_pass,
    nvs_fname,
    nvs_broker_ip,
    nvs_seat_number,
    nvs_part_ver
} CV_NVS_KEY;
extern bool get_nvs_value(CV_NVS_KEY nvs_key);
extern bool set_nvs_u8val(CV_NVS_KEY nvs_key, uint8_t val);
extern bool set_nvs_strval(CV_NVS_KEY nvs_key, char* val);
extern uint8_t get_part_ver(CV_NVS_KEY nvs_key);