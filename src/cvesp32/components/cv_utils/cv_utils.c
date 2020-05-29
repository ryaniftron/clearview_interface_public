#include "cv_utils.h"
#include "esp_system.h"
#include "string.h"
#include "esp_event.h"
#include <esp_log.h>
#include "nvs.h"
#include "nvs_flash.h"

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

char desired_ap_ssid[WIFI_CRED_MAXLEN];
char desired_ap_pass[WIFI_CRED_MAXLEN];
char desired_friendly_name[WIFI_CRED_MAXLEN];
char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];
int node_number = 0;

static bool _nvs_is_init = false;


bool nn_update_needed = false; //node number needs to be updated

extern bool start_nvs(){
    if (_nvs_is_init) {
        ESP_LOGW(TAG_UTILS, "NVS is already initialized");
        return false;
    }

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK( err );
    _nvs_is_init = true;
    return true;
}

static void check_nvs_init(void){
    if (!_nvs_is_init) {
        ESP_LOGI(TAG_UTILS, "NVS is being autoinitialized");
        start_nvs();
    }
}

extern bool read_nvs_value(void){

    check_nvs_init();

    // Open
    nvs_handle_t my_handle;
    esp_err_t err  = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%04x) opening NVS handle!\n", err);
    } else {
        printf("Done\n");

        // Read
        printf("Reading restart counter from NVS ... ");
        uint8_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_u8(my_handle, "restart_counter", &restart_counter);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Restart counter = %d\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }


    return false;
}



extern void get_chip_id(char* ssid, const int UNIQUE_ID_LENGTH){
    //TODO it's reverse order. Try this instead https://github.com/espressif/arduino-esp32/issues/932#issuecomment-352307067
    uint64_t chipid = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&chipid));
    uint16_t chip = (uint16_t)(chipid >> 32);
    //esp_read_mac(chipid);
    snprintf(ssid, UNIQUE_ID_LENGTH, "CV_%04X%08X", chip, (uint32_t)chipid);
    printf("SSID created from chip id: %s\n", ssid);
    return;
}

extern void remove_ctrlchars(char *str) 
{ 
    // To keep track of non-space character count 
    int count = 0; 
  
    // Traverse the given string. If current character 
    // is not space, then place it at index 'count++' 
    for (int i = 0; str[i]; i++) 
        if (str[i] >= 30) 
            str[count++] = str[i]; // here count is 
                                   // incremented 
    str[count] = '\0'; 
} 


// Return true if the crediential was set
extern bool set_credential(char* credentialName, char* val){
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
            int new_node_number = atoi(val);
            if (node_number == new_node_number){
                ESP_LOGW(TAG_UTILS, "No change in node_number");
                return false;
            }
            node_number = atoi(val);
        } else {
            ESP_LOGE(TAG_UTILS, "Ignoring node_number out of range");
            return false;
        }
        
    } else {
        ESP_LOGE(TAG_UTILS, "Unexpected Credential of %s", credentialName);
        return false;
    }

    //Todo store the value in eeprom
    return true; //successful value set
}