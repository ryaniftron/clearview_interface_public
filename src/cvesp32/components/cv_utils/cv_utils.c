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
uint8_t desired_node_number;

#define NVS_KSIZE 16 //Including null
static bool _nvs_is_init = false;

bool nn_update_needed = false; //node number needs to be updated

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
        set_nvs_strval(nvs_fname, val); //no checks needed. set directly here
    } else if (strcmp(credentialName, "broker_ip") == 0) {
        strcpy(desired_mqtt_broker_ip, val);
    } else if (strcmp(credentialName, "node_number") == 0) {
        if (0 <= atoi(val) && atoi(val) <= 7){
            int new_node_number = atoi(val);
            if (desired_node_number == new_node_number){
                ESP_LOGW(TAG_UTILS, "No change in node_number");
                return false;
            }
            desired_node_number = atoi(val);
            set_nvs_u8val((CV_NVS_KEY) nvs_node_number, desired_node_number);
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


// **************
//     NVS
// **************

// Performs all NVS initialization
// Loads NVS variables into program memory or uses defaults if they don't exist
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
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    _nvs_is_init = true; //TODO if failure to initialize, should this be false?
    return load_all_nvs();
}

static void check_nvs_init(void){
    if (!_nvs_is_init) {
        ESP_LOGI(TAG_UTILS, "NVS is being autoinitialized");
        start_nvs();
        ESP_LOGI(TAG_UTILS, "NVS is initialized");
    }
}

static bool open_nvs_handle(nvs_handle_t *nvs_handle){
    bool nvs_open_succ;
    check_nvs_init();
    esp_err_t err  = nvs_open("storage", NVS_READWRITE, nvs_handle);

    if (err != ESP_OK) {
        printf("Error (%04x) opening NVS handle!\n", err);
        nvs_open_succ = false;
    } else {
        printf("NVS OPEN SUCC\n");
        nvs_open_succ = true;
    }
    return nvs_open_succ;
}

static void get_nvs_str_from_enum(CV_NVS_KEY k, char* nvs_str) {
    switch (k) {
        case nvs_wifi_ssid:
            strcpy(nvs_str,"wssid");
            break;
        case nvs_wifi_pass:
            strcpy(nvs_str, "wpass");
            break;
        case nvs_fname:
            strcpy(nvs_str,"fname");
            break;
        case nvs_broker_ip:
            strcpy(nvs_str,"broker_ip");
            break;
        case nvs_node_number:
            strcpy(nvs_str,"node_num");
            break;
        default:
            ESP_LOGE(TAG_UTILS, "Invalid NVS key");
            strcpy(nvs_str,"");
            break;
    }
}

static bool initialize_default_nvs(CV_NVS_KEY k, nvs_handle_t nh){
    esp_err_t err;
    char kstr[NVS_KSIZE]; 
    get_nvs_str_from_enum(k, kstr);

    switch (k) {
        case nvs_wifi_ssid:
            err = nvs_set_str(nh, kstr, CONFIG_NETWORK_SSID);
            break;
        case nvs_wifi_pass:
            err = nvs_set_str(nh, kstr, CONFIG_NETWORK_PASSWORD);
            break;
        case nvs_fname:
            err = nvs_set_str(nh, kstr, CONFIG_FRIENDLY_NAME);
            break;
        case nvs_broker_ip:
            err = nvs_set_str(nh, kstr, CONFIG_BROKER_IP);
            break;
        case nvs_node_number:
            err = nvs_set_u8(nh, kstr, CONFIG_NODE_NUMBER);
            break;
        default:
            ESP_LOGE(TAG_UTILS, "Invalid NVS key");
            err = ESP_ERR_NVS_NOT_FOUND;
            break;
    }
    return err;
}

//return true if value loaded from NVS or it was loaded from default
extern bool get_nvs_value(CV_NVS_KEY nvs_key){
    esp_err_t err = ESP_OK;
    bool get_nvs_succ = false;
    // Open
    nvs_handle_t nh;
    open_nvs_handle(&nh);

    char kstr[NVS_KSIZE]; 
    get_nvs_str_from_enum(nvs_key, kstr);

    // Read
    printf("Reading %s from NVS ... ", kstr);

    size_t required_size;
    char* str_val; //for string returns
    uint8_t u8_val; //for u8 returns
    switch (nvs_key) {
        case nvs_wifi_ssid:
            nvs_get_str(nh, kstr, NULL, &required_size);
            str_val = malloc(required_size);
            err = nvs_get_str(nh, kstr, str_val, &required_size);
            if (err==ESP_OK){
                strcpy(desired_ap_ssid, str_val);
                printf("'%s'", str_val);
            }
            free(str_val);
            break;
        case nvs_wifi_pass:
            nvs_get_str(nh, kstr, NULL, &required_size);
            str_val = malloc(required_size);
            err = nvs_get_str(nh, kstr, str_val, &required_size);
            if (err==ESP_OK){
                strcpy(desired_ap_pass, str_val);
                printf("'%s'", str_val);
            }
            free(str_val);
            break;
        case nvs_fname:
            nvs_get_str(nh, kstr, NULL, &required_size);
            str_val = malloc(required_size);
            err = nvs_get_str(nh, kstr, str_val, &required_size);
            if (err==ESP_OK){
                strcpy(desired_friendly_name, str_val);
                printf("'%s'", str_val);
            }
            free(str_val);
            break;
        case nvs_broker_ip:
            nvs_get_str(nh, kstr, NULL, &required_size);
            str_val = malloc(required_size);
            err = nvs_get_str(nh, kstr, str_val, &required_size);
            if (err==ESP_OK){
                strcpy(desired_mqtt_broker_ip, str_val);
                printf("'%s'", str_val);
            }
            free(str_val);
            break;
        case nvs_node_number:
            err = nvs_get_u8(nh, kstr, &u8_val);
            if (err==ESP_OK){
                desired_node_number = u8_val;
                printf("'%u'", u8_val);
            }
            break;
        default:
            ESP_LOGE(TAG_UTILS, "Invalid NVS key");
            return false;
            break;
    }
     
    switch (err) {
        case ESP_OK:
            printf("Success getting value. \n");
            get_nvs_succ = true;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet. Using default.\n");
            get_nvs_succ = initialize_default_nvs(nvs_key, nh);
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
            get_nvs_succ = false;
    }
    nvs_close(nh);
    return get_nvs_succ;
}

static bool load_all_nvs(void){
    //Return true if all values loaded successfully
    bool load_succ = false;
    load_succ &= get_nvs_value(nvs_wifi_ssid);
    load_succ &= get_nvs_value(nvs_wifi_pass);
    load_succ &= get_nvs_value(nvs_fname);
    load_succ &= get_nvs_value(nvs_broker_ip);
    load_succ &= get_nvs_value(nvs_node_number);
    return load_succ;
}


extern bool set_nvs_strval(CV_NVS_KEY nvs_key, char* val) {
    bool nvs_set_succ = true;
    esp_err_t err = ESP_OK;

    nvs_handle_t nh;
    open_nvs_handle(&nh);

    char kstr[NVS_KSIZE]; 
    get_nvs_str_from_enum(nvs_key, kstr);

    err = nvs_set_str(nh, kstr, val);
    ESP_LOGI(TAG_UTILS, "NVS set string of %s to %s %s", kstr, val, (err==ESP_OK)?"successful":"failed");
    if (err!=ESP_OK){nvs_set_succ=false;}

    // // Commit written value.
    // // After setting any values, nvs_commit() must be called to ensure changes are written
    // // to flash storage. Implementations may write to storage at other times,
    // // but this is not guaranteed.

    err = nvs_commit(nh);
    ESP_LOGI(TAG_UTILS, "%s",(err != ESP_OK) ? "Failed to commit!\n" : "Done Committing\n");
    // if (err!=ESP_OK){nvs_set_succ=false;}
    return nvs_set_succ;
}

extern bool set_nvs_u8val(CV_NVS_KEY nvs_key, uint8_t val) {
    bool nvs_set_succ = true;
    esp_err_t err = ESP_OK;

    nvs_handle_t nh;
    open_nvs_handle(&nh);

    char kstr[NVS_KSIZE]; 
    get_nvs_str_from_enum(nvs_key, kstr);

    err = nvs_set_u8(nh, kstr, val);
    ESP_LOGI(TAG_UTILS, "NVS set string of %s to %u %s", kstr, val, (err==ESP_OK)?"successful":"failed");
    if (err!=ESP_OK){nvs_set_succ=false;}

    // // Commit written value.
    // // After setting any values, nvs_commit() must be called to ensure changes are written
    // // to flash storage. Implementations may write to storage at other times,
    // // but this is not guaranteed.

    err = nvs_commit(nh);
    ESP_LOGI(TAG_UTILS, "%s",(err != ESP_OK) ? "Failed to commit!\n" : "Done Committing\n");
    // if (err!=ESP_OK){nvs_set_succ=false;}
    return nvs_set_succ;
}