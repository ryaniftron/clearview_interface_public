#ifndef CV_UTILS_C
#define CV_UTILS_C
#include "esp_system.h"
#include "string.h"
#include "esp_event.h"
#include <esp_log.h>

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

// parse the request payload for credentials for the wifi device
// Successful parse returns true
// static bool parse_net_credentials(char* payload){
//     ///example: /config_esp32?ssid=a&password=x&device_name=z
//     printf("Parsing these creds: %s\n", payload);

//     char credentialSearchTerms[5][15] = {"ssid","password","device_name","broker_ip","\0"};

//     char *start,*stop,*arg1, *k, *knext; 
    
//     int nConfTerms = 4;
//     for (int i = 0 ; i < nConfTerms ; i++){
//         k = credentialSearchTerms[i];
//         //knext = credentialSearchTerms[i+1];
//         knext = "&";
//         printf("\tGetting Param %s\n", k);
//         start = strstr(payload, k);
//         if (start == NULL){
//             printf("Error. Start parameter not found in %s\n", payload);
//             return false;
//         }
//         start += 1 + strlen(k);
//         stop = strstr(start + 1,  knext);
//         printf("\t\tStart:%s\n",start);
        

//         if (stop == NULL) {
//             if (i == nConfTerms - 1){ //last credential to parse.
//                 printf("DEBUG: End term in %s\n", payload); 
//                 set_credential(k,start);
//                 printf("DEBUG2\n");
//             } else {
//                 printf("Error. End parameter not found in %s\n", payload);
//                 return false;
//             }

//         }else{
//             printf("\t\tStop:%s\n",stop);
//             stop = stop; //remove the '='

//             // allocate memory for the payload
//             arg1 = (char *) malloc(stop - start + 1);
//             memcpy(arg1, start, stop - start);
//             arg1[stop - start] = '\0';
//             printf("\targ1:%s\n", arg1);
//             set_credential(k,arg1);
//             free(arg1);
//         }
//     }
//     extern bool switch_to_sta;
//     switch_to_sta = true;
//     return true;
// }

#endif // CV_UTILS_C