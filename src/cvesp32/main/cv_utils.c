#ifndef CV_UTILS_C
#define CV_UTILS_C
#include "esp_system.h"
#include "string.h"
#include "esp_event.h"
#include <esp_log.h>

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

//macros to log with function name and line number
#define LOG_FMT_FUNCLNAME(x )     "%s: " x, __func__, __line__
   ///(__FILE__, __LINE__, __func__, ...);
// #define CV_LOGEL(x)  // error (lowest)
// #define CV_LOGWL(x)  // warning
// #define CV_LOGIL(x)  // info
// #define CV_LOGDL(x)  // debug
// #define CV_LOGVL(x)  // verbose (highest)

const char* TAG_UTILS = "CV_UTILS";


#define WIFI_CRED_MAXLEN 32
char desired_ap_ssid[WIFI_CRED_MAXLEN];
char desired_ap_pass[WIFI_CRED_MAXLEN];
char desired_friendly_name[WIFI_CRED_MAXLEN];
char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];
int node_number = 0;

static void get_chip_id(char* ssid, const int UNIQUE_ID_LENGTH){
    uint64_t chipid = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&chipid));
    uint16_t chip = (uint16_t)(chipid >> 32);
    //esp_read_mac(chipid);
    snprintf(ssid, UNIQUE_ID_LENGTH, "CV_%04X%08X", chip, (uint32_t)chipid);
    printf("SSID created from chip id: %s\n", ssid);
    return;
}



static void set_credential(char* credentialName, char* val){
    printf("##Setting Credential %s = %s\n", credentialName, val);
    if (strlen(val) > WIFI_CRED_MAXLEN) {
        ESP_LOGE(TAG_UTILS, "\t Unable to set credential. Too long.");
    }

    if (strcmp(credentialName, "ssid") == 0) {
        strcpy(desired_ap_ssid, val);
    } else if (strcmp(credentialName, "password") == 0) {
        strcpy(desired_ap_pass, val);
    } else if (strcmp(credentialName, "device_name") == 0) {
        strcpy(desired_friendly_name, val);
    } else if (strcmp(credentialName, "broker_ip") == 0) {
        strcpy(desired_mqtt_broker_ip, val);
    } else if (strcmp(credentialName, "node_number") == 0) {
        if (0 <= atoi(val) && atoi(val) <= 7){
            node_number = atoi(val);
            ESP_LOGI(TAG_UTILS, "TODO change node subscriptions");
        } else {
            ESP_LOGW(TAG_UTILS, "Ignoring node_number out of range");
        }
        
    } else {
        ESP_LOGE(TAG_UTILS, "Unexpected Credential of %s", credentialName);
        return;
    }

    //Todo store the value in eeprom

}


#endif // CV_UTILS_C