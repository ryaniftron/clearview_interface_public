
#define ignore_unused_function_and_vars
#ifdef ignore_unused_function_and_vars
	#pragma GCC diagnostic ignored "-Wunused-function" 
	#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_wifi.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "tcpip_adapter.h"



#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

//Components

#include "cv_utils.c"


#include "main_cv_mqtt.c"
#if CONFIG_ENABLE_MQTT != 1
    #warning CV MQTT is disabled
#endif // CONFIG_ENABLE_MQTT != 1


#include "cv_ledc.c"
#if CONFIG_ENABLE_LED != 1
    #warning CV LED is disabled
#endif //CONFIG_ENABLE_LED != 1

#define WEB_SERVER_ON CONFIG_WEB_SERVER_BOTH || CONFIG_WEB_SERVER_SOFTAP_ONLY || CONFIG_WEB_SERVER_STA_ONLY
#include "cv_server.c"
#if WEB_SERVER_ON != 1
    #warning CV Web Server is disabled
#endif //WEB_SERVER_ON != 1



#if CONFIG_ENABLE_SERIAL
    #include "cv_uart.c"
#else
    #warning CV_UART is disabled
#endif //CONFIG_ENABLE_SERIAL



// Hardware selection
#ifndef HW_ESP_WROOM_32
    #define HW_ESP_WROOM_32 1
#endif

#ifndef HW_ESP_WROOM_32D
    #define HW_ESP_WROOM_32D 2
#endif

#ifndef HW
    //#define HW HW_ESP_WROOM_32
    #define HW HW_ESP_WROOM_32
#endif

#define WIFI_ON CONFIG_SOFTAP_ALLOWED || CONFIG_STA_ALLOWED || CONFIG_BOTH_WIFI_ALLOWED
#if WIFI_ON == 1
#warning "WIFI ON"
#else
#warning "WIFI OFF"
#endif






//****************
// UART Testing
//****************
 
#ifndef UART_TEST_LOOP
    //uncomment this to run the test UART command periodically.
    //#define UART_TEST_LOOP 1
#endif


//*****************
//Network credentials for station mode. 
//*****************

//If uncommended, these override Kconfig.projbuild
//#define my_network_ssid           "my_ssid_override"
//#define my_network_pass           "my_pwd_override"

// the run "idf.py menuconfig" and  navigate to example configuration to change your defaults
#ifndef my_network_ssid
    #define AP_TARGET_SSID            CONFIG_NETWORK_SSID
#else
    #define AP_TARGET_SSID            my_network_ssid
#endif

#ifndef my_network_ssid
    #define AP_TARGET_PASS            CONFIG_NETWORK_PASSWORD
#else
    #define AP_TARGET_PASS            my_network_pass
#endif

//AP (softAP) configuration -> See main/Kconfig.projbuild
#define SOFTAP_SSID               CONFIG_ESP_WIFI_SSID
#define SOFTAP_PASS               CONFIG_ESP_WIFI_PASSWORD
#define SOFTAP_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN


static const char *TAG = "cv-esp32";

#if WIFI_ON == 1
static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;

bool switch_to_sta = false;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0; //current number of retries ESP32 has tried to connect to AP
#define ESP_MAXIMUM_RETRY 3

// This gets flagged true if wifi failed to connect
// Can be used to notify user of failed connection
static bool wifi_connect_fail = false;


/*
typedef enum {
    CV_AP_INIT = 0,                                 // Started up for the first time (ever)  
    CV_STA_CONN_ATTEMPT,                            // Attempting Connection as STA to network 
    CV_STA_CONN_NO_BROKER,            //  MQTT connection refused reason: ID rejected 
    CV_STA_CONN_WITH_BROKER,     //  MQTT connection refused reason: Server unavailable 
    CV_AP_FAIL_,           //  MQTT connection refused reason: Wrong user 
    MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED          //  connection refused reason: Wrong username or password 
} esp_mqtt_connect_return_code_t;
*/




#define HDR_200 "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"
#define HDR_201 "HTTP/1.1 201 Created\r\nContent-type: text/html\r\n\r\n"
#define HDR_204 "HTTP/1.1 204 No Content\r\nContent-type: text/html\r\n\r\n"
#define HDR_404 "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n"
#define HDR_405 "HTTP/1.1 405 Method not allowed\r\nContent-type: text/html\r\n\r\n"
#define HDR_409 "HTTP/1.1 409 Conflict\r\nContent-type: text/html\r\n\r\n"
#define HDR_501 "HTTP/1.1 501 Not Implemented\r\nContent-type: text/html\r\n\r\n"
//#define root_text "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br> "


// TODO improve file serving like in here: https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/file_serving/main/file_server.c
// https://github.com/espressif/esp-idf/blob/a2263571b5ffb4071c4d4abbd8115e0f694dd9fe/examples/protocols/http_server/simple/main/main.c
#define root_text \
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <strong>ClearView ESP32 Config </strong><br> \
    Wifi Status: TODO <br>\
    <form action=\"/config_esp32\" method> \
        <label for=\"ssid\">Network SSID:</label><br> \
        <input type=\"text\" id=\"ssid\" name=\"ssid\"><br> \
        <label for=\"password\">Network Password:</label><br> \
        <input type=\"text\" id=\"password\" name=\"password\"><br> \
        <label for=\"device_name\">Device Name:</label><br> \
        <input type=\"text\" id=\"device_name\" name=\"device_name\"><br> \
        <label for=\"broker_ip\">Broker IP Address:</label><br> \
        <input type=\"text\" id=\"broker_ip\" name=\"broker_ip\"><br> \
        <input type=\"submit\" value=\"Save and Join Network\">\
    *note: Leave empty for defaults<br> \
"

#define test_page_text \
    "<strong>ClearView ESP32 Test Page </strong><br> \
    <form action=\"/ask_for_lock\" method> \
        <label for=\"req_lock\">Lock?</label><br> \
        <input type=\"submit\" value=\"Test UART\">\
"

#define fail_connect_text \
    "Failed to connect to wifi<br> \
    Attempted SSID: TODO<br> \
    Attemped Password: TODO<br> \
    <form action=\"/\" method> \
        <input type=\"submit\" value=\"Return to Main\">\
    </form> \
"


//Add it to root text when the functionality is ready
//this is the scan for wifi button if needed. 
/* 

    </form> \
            \
    <form action=\"/scan_wifi\" method> \
        <input type=\"submit\" value=\"Scan for WiFi\">\
    </form> \
*/

#define wifi_scan_text \
    " Scanned Wifi List: <br> \
    <form action=\"/\" method> \
        <input type=\"submit\" value=\"Return to main\">\
    </form> \
    "

#define no_connection_to_ap_text \
    " No connection to wifi : <br> \
    ssid: %s <br>\
    password: %s <br> \
    <form action=\"/\" method> \
        <input type=\"submit\" value=\"Return to main\">\
    </form> \
    "

#define wifi_attempt_connect \
    " Attempting wifi connection... <br>\
      This hotspot will be turning off unless connection is failed.\
    "
    
#define config_esp32_parse_fail \
    " Error: config_esp32 Parse Fail <br> \
    "

static void ap_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));

        ip4_addr_t cv_ip;
        
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}



static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
			break;
		default:
			break;
	}
	return ESP_OK;
}

//TODO combine this with event_handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}



static void initialize_softAP_wifi(char* PARAM_ESP_WIFI_SSID, uint8_t PARAM_SSID_LEN)
{
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html
    printf("Starting ESP32 softAP mode.\n");
    if (wifi_connect_fail){
        set_ledc_code(0, led_blink_fast);
    } else {
        set_ledc_code(0, led_blink_slow);
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //VSCODE shows this undefined but it works
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SOFTAP_SSID,
            .ssid_len = 0,
            .password = SOFTAP_PASS, //This is either empty string or >= 8 chars or it breaks
            .max_connection = SOFTAP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    //Set the SSID to PARAM_ESP_WIFI_SSID
    strcpy((char *)wifi_config.ap.ssid, PARAM_ESP_WIFI_SSID);

    if (strlen(SOFTAP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    if (strlen(SOFTAP_PASS) == 0){
        ESP_LOGI(TAG, "softAP started. SSID:'%s'",
             wifi_config.ap.ssid);
        ESP_LOGI(TAG,"\t**unsecured - no password required**\n");
    } else {
        ESP_LOGI(TAG, "softAP started. SSID:'%s' password:'%s'",
             wifi_config.ap.ssid, wifi_config.ap.password);
             //TODO password connection/security may not work well
    }
      
    # if WEB_SERVER_ON
        start_cv_webserver();
    #endif //WEB_SERVER_ON
}

// Connect as station to AP
// Returns true on success, false on fail
static bool initialise_sta_wifi(char* PARAM_HOSTNAME)
{    
    if (s_wifi_event_group != NULL) {
        ESP_LOGE(TAG, "s_wifi_event_group is not NULL");
    }

    s_wifi_event_group = xEventGroupCreate();
    set_ledc_code(0, led_on);

    ESP_ERROR_CHECK(esp_event_loop_create_default()); //only once

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ap_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = AP_TARGET_SSID,
            .password = AP_TARGET_PASS
        },
    };

    // Use default SSID and PASS unless supplied in config
    //if (strcmp("", desired_ap_ssid) == 0) {
    strcpy((char *)wifi_config.sta.ssid, desired_ap_ssid);
    //if (strcmp("", desired_ap_pass) == 0) {
    strcpy((char *)wifi_config.sta.password, desired_ap_pass);
    //}

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_LOGI(TAG, "WIFI Ready to start");
    
    ESP_ERROR_CHECK(esp_wifi_start() );
    
    /*esp_err_t ret = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA ,PARAM_HOSTNAME);
    if(ret != ESP_OK ){
      ESP_LOGE(TAG,"failed to set hostname:%d",ret);  
    } else {
        ESP_LOGI(TAG, "hostname has been set as %s", PARAM_HOSTNAME);
    }
    */
    ESP_LOGI(TAG, "wifi_init_sta finished. Connecting to SSID:%s password:%s as %s",
             wifi_config.sta.ssid, wifi_config.sta.password,PARAM_HOSTNAME);

    // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    // number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    //  happened. 
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid,wifi_config.sta.password);
        wifi_connect_fail = false;
        set_ledc_code(0, led_breathe_fast);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid,wifi_config.sta.password);
        wifi_connect_fail = true;
        
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        wifi_connect_fail = true;
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ap_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler));
    vEventGroupDelete(s_wifi_event_group);
    
    return !wifi_connect_fail;
}

//disconnect and kill wifi driver no matter which mode it is in
void kill_wifi(void){
    #if HW == HW_ESP_WROOM_32
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));

    if (mode == WIFI_MODE_STA){ ///TODO this condition needs testing on all three modes
        //Answer the question: When do you need to run esp_wifi_disconnect. 
        //Confirmed: Don't run in WIFI_MODE_AP because there is nothing to disconnect from and it breaks.
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    #elif HW == HW_ESP_WROOM_32D
        printf("Can't kill wifi in 32D\n");
        ESP_ERROR_CHECK(esp_event_loop_delete_default());
    #endif

}


static void demo_sequential_wifi(char* PARAM_ESP_WIFI_SSID, uint8_t PARAM_SSID_LEN)
{
    printf("Starting ESP32 both mode.\n"); 
    initialize_softAP_wifi(PARAM_ESP_WIFI_SSID,PARAM_SSID_LEN);
    //printf("Starting http server\n");
    //xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);

    


    printf("Waiting for  switch_to_sta\n");;
    while (true){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (switch_to_sta == true)
        {   
            printf("Switching from softAP to station\n");
            switch_to_sta = false;
           
            break;
        }
    }
    kill_wifi();
    
    #if HW == HW_ESP_WROOM_32
        bool sta_succ = initialise_sta_wifi(PARAM_ESP_WIFI_SSID);
        if (!sta_succ) { 
            //restart sequential wifi
            kill_wifi();
            demo_sequential_wifi(PARAM_ESP_WIFI_SSID,PARAM_SSID_LEN);
        }
        
    #elif HW == HW_ESP_WROOM_32D
        printf("Not restarting wifi yet...\n");
        bool sta_succ = initialise_sta_wifi(PARAM_ESP_WIFI_SSID);
    #endif
}

#endif // WIFI_ON



void app_main(void)
{
	// //Initialize NVS
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     // NVS partition was truncated and needs to be erased
    //     // Retry nvs_flash_init
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();
    const int UNIQUE_ID_LENGTH = 16;
    char chipid[UNIQUE_ID_LENGTH];
    get_chip_id(chipid, UNIQUE_ID_LENGTH);


    CV_LED_Code_t initial_led_state = led_off;
    init_cv_ledc(initial_led_state);


      
    
    #ifdef UART_TEST_LOOP
        run_cv_uart_test_task();
    #else //Normal program logic
        //Start Wifi
        #if CONFIG_STARTUP_WIFI_STA | CONFIG_STA_ALLOWED
            strcpy(desired_ap_ssid, AP_TARGET_SSID);
            strcpy(desired_ap_pass, AP_TARGET_PASS);
            strcpy(desired_mqtt_broker_ip, CONFIG_BROKER_IP);
            strcpy(desired_friendly_name, CONFIG_FRIENDLY_NAME);
            //strcpy(node_number, CONFIG_NODE_NUMBER);
            initialise_sta_wifi(chipid);
        #elif CONFIG_STARTUP_WIFI_SOFTAP 
            demo_sequential_wifi(chipid, UNIQUE_ID_LENGTH); //this returns on successful connection
        #elif CONFIG_SOFTAP_ALLOWED
            initialize_softAP_wifi(chipid, UNIQUE_ID_LENGTH);
        #endif //SKIP_SOFTAP
        //only after in sequential wifi do we start mqtt. Give it some time
        vTaskDelay(2000 / portTICK_PERIOD_MS);


            init_uart();

            //run_cvuart_rx_task();

        
        #if CONFIG_ENABLE_MQTT
            cv_mqtt_init(chipid, UNIQUE_ID_LENGTH, desired_mqtt_broker_ip);
        #endif //CONFIG_ENABLE_MQTT == 1
    #endif // UART_TEST_LOOP
    
    printf("END OF MAIN\n");
}




