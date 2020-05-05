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
static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;


#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

//Components
//#include "cv_mqtt.h"
#include "main_cv_mqtt.c"


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

#define WIFI_CRED_MAXLEN 32
char desired_ap_ssid[WIFI_CRED_MAXLEN];
char desired_ap_pass[WIFI_CRED_MAXLEN];
char desired_friendly_name[WIFI_CRED_MAXLEN];
char desired_mqtt_broker_ip[WIFI_CRED_MAXLEN];





#define HDR_200 "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"
#define HDR_201 "HTTP/1.1 201 Created\r\nContent-type: text/html\r\n\r\n"
#define HDR_204 "HTTP/1.1 204 No Content\r\nContent-type: text/html\r\n\r\n"
#define HDR_404 "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n"
#define HDR_405 "HTTP/1.1 405 Method not allowed\r\nContent-type: text/html\r\n\r\n"
#define HDR_409 "HTTP/1.1 409 Conflict\r\nContent-type: text/html\r\n\r\n"
#define HDR_501 "HTTP/1.1 501 Not Implemented\r\nContent-type: text/html\r\n\r\n"
//#define root_text "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br> "


// TODO improve file serving like in here: https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/file_serving/main/file_server.c
#define root_text \
    "<strong>ClearView ESP32 Config </strong><br> \
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


static void set_credential(char* credentialName, char* val){
    printf("##Setting Credential %s = %s\n", credentialName, val);
    if (strlen(val) > WIFI_CRED_MAXLEN) {
        ESP_LOGE(TAG, "\t Unable to set credential.");
    }

    if (strcmp(credentialName, "ssid") == 0) {
        strcpy(desired_ap_ssid, val);
    } else if (strcmp(credentialName, "password") == 0) {
        strcpy(desired_ap_pass, val);
    } else if (strcmp(credentialName, "device_name") == 0) {
        strcpy(desired_friendly_name, val);
    } else if (strcmp(credentialName, "broker_ip") == 0) {
        strcpy(desired_mqtt_broker_ip, val);
    } else {
        ESP_LOGE(TAG, "Unexpected Credential of %s", credentialName);
        return;
    }

    //Todo store the value in eeprom

}

// parse the request payload for credentials for the wifi device
// Successful parse returns true
static bool parse_net_credentials(char* payload){
    ///example: /config_esp32?ssid=a&password=x&device_name=z
    printf("Parsing these creds: %s\n", payload);

    char credentialSearchTerms[4][15] = {"ssid","password","device_name","broker_ip","\0"};

    char *start,*stop,*arg1, *k, *knext; 
    
    int nConfTerms = 4;
    for (int i = 0 ; i < nConfTerms ; i++){
        k = credentialSearchTerms[i];
        //knext = credentialSearchTerms[i+1];
        knext = "&";
        printf("\tGetting Param %s\n", k);
        start = strstr(payload, k);
        if (start == NULL){
            printf("Error. Start parameter not found in %s\n", payload);
            return false;
        }
        start += 1 + strlen(k);
        stop = strstr(start + 1,  knext);
        printf("\t\tStart:%s\n",start);
        

        if (stop == NULL) {
            if (i == nConfTerms - 1){ //last credential to parse.
                printf("DEBUG: End term in %s\n", payload); 
                set_credential(k,start);
                printf("DEBUG2\n");
            } else {
                printf("Error. End parameter not found in %s\n", payload);
                return false;
            }

        }else{
            printf("\t\tStop:%s\n",stop);
            stop = stop; //remove the '='

            // allocate memory for the payload
            arg1 = (char *) malloc(stop - start + 1);
            memcpy(arg1, start, stop - start);
            arg1[stop - start] = '\0';
            printf("\targ1:%s\n", arg1);
            set_credential(k,arg1);
            free(arg1);
        }
    }
    switch_to_sta = true;
    return true;
}

static void
http_server_netconn_serve(struct netconn *conn)
{
	struct netbuf *inbuf;
	char *buf, *payload, *start, *stop;
	u16_t buflen;
	err_t err;

   // Read the data from the port, blocking if nothing yet there.
   // We assume the request (the part we care about) is in one netbuf 
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
        // TODO capture the possible error code form netbuf_data
		netbuf_data(inbuf, (void**)&buf, &buflen);
		// find the first and seconds space (those delimt the url)
		start = strstr(buf, " ") + 1;
		stop = strstr(start + 1, " ");	
		// allocate memory for the payload
		payload = (char *) malloc(stop - start + 1);
		memcpy(payload, start, stop - start);
		payload[stop - start] = '\0';

		// For now  only GET results in a valid response
		if (strncmp(buf, "GET /", 5) == 0){
            netconn_write(conn, HDR_200, sizeof(HDR_200)-1, NETCONN_NOCOPY);
			//printf("GET = '%s' \n", payload);
            if (strcmp(payload, "/") == 0 || strcmp(payload, "/?") == 0) { //give the root website
                //TODO instead of using a global bool, figure out something more robust
                if (wifi_connect_fail){
                    netconn_write(conn, fail_connect_text, sizeof(fail_connect_text)-1, NETCONN_NOCOPY);
                    wifi_connect_fail = false; //reset fail to connect so a user can try again
                } else {
                    netconn_write(conn, root_text, sizeof(root_text)-1, NETCONN_NOCOPY);
                }
                
            }else if (strcmp(payload, "/favicon.ico") == 0) {
                //do nothing
            }else if (strcmp(payload, "/scan_wifi?") == 0) {
                netconn_write(conn, wifi_scan_text, sizeof(wifi_scan_text)-1, NETCONN_NOCOPY);
            }else if (strncmp(payload, "/config_esp32", strlen("/config_esp32")) == 0){
                printf("Got a config filestring\n");
                //GET='GET /config_esp32?ssid=A&password=B&device_name=C '
                if (parse_net_credentials(payload)){
                    netconn_write(conn, wifi_attempt_connect, sizeof(wifi_attempt_connect)-1, NETCONN_NOCOPY);
                    switch_to_sta = true;
                }else {
                    netconn_write(conn, config_esp32_parse_fail, sizeof(config_esp32_parse_fail)-1, NETCONN_NOCOPY);
                }
            }else{  
                //char* errmsg = sprintf("Uncaptured GET='%s' \n", *payload);
                //printf("Uncaptured GET='%s' \n", &payload);
                //netconn_write(conn, errmsg, sizeof(errmsg)-1, NETCONN_NOCOPY);              
            }

            
                       
			
		}else if (strncmp(buf, "POST /", 6) == 0){
			// send '501 Not implementd' reply  
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "PUT /", 5) == 0){
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "PATCH /", 7) == 0){
			// send '501 Not implementd' reply  
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "DELETE /", 8) == 0){
			// send '501 Not implementd' reply  
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else{
			// 	Any unrecognized verb will automatically 
			//	result in '501 Not implementd' reply 
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}
		free(payload);
	}
  	// Close the connection (server closes in HTTP) and clean up after ourself 
	netconn_close(conn);
	netbuf_delete(inbuf);
}


static void http_server(void *pvParameters)
{
    
	uint32_t port;
	if (pvParameters == NULL){
		port = 80;
	}else{
		port = (uint32_t) pvParameters;
	}
	// printf("\n Creating socket at\n %d \n", (uint32_t) pvParameters);
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL,  port);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while(err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
    
}

static void get_chip_id(char* ssid, const int UNIQUE_ID_LENGTH){
    uint64_t chipid = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&chipid));
    uint16_t chip = (uint16_t)(chipid >> 32);
    //esp_read_mac(chipid);
    snprintf(ssid, UNIQUE_ID_LENGTH, "CV_%04X%08X", chip, (uint32_t)chipid);
    printf("SSID created from chip id: %s\n", ssid);
    return;
}


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
    ESP_LOGI(TAG, "Login and go to http://192.168.4.1/ \n");
    

}

// Connect as station to AP
// Returns true on success, false on fail
static bool initialise_sta_wifi(char* PARAM_HOSTNAME)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default()); //only once

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ap_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = AP_TARGET_SSID,
            .password = AP_TARGET_PASS
        },
    };

    // Use default SSID and PASS unless supplied in config
    //if (strcmp("", desired_ap_ssid) == 0) {
    strcpy((char *)wifi_config.sta.ssid, desired_ap_ssid);
    ESP_LOGI(TAG, "ESP WIFI Desired %s",desired_ap_ssid );
    ESP_LOGI(TAG, "ESP WIFI Actual %s",wifi_config.sta.ssid );
    //if (strcmp("", desired_ap_pass) == 0) {
    strcpy((char *)wifi_config.sta.password, desired_ap_pass);
    //}

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    esp_err_t ret = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA ,PARAM_HOSTNAME);
    if(ret != ESP_OK ){
      ESP_LOGE(TAG,"failed to set hostname:%d",ret);  
    } else {
        ESP_LOGI(TAG, "hostname has been set as %s", PARAM_HOSTNAME);
    }

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
}

static void demo_sequential_wifi(char* PARAM_ESP_WIFI_SSID, uint8_t PARAM_SSID_LEN)
{
    printf("Starting ESP32 both mode.\n"); 
    initialize_softAP_wifi(PARAM_ESP_WIFI_SSID,PARAM_SSID_LEN);
    printf("Starting http server\n");
    xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
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
    printf("Onward! esp_wifi_stop has been run. Time to start station.... (TODO) \n ");
    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-deinit-phase

    kill_wifi();
    
    bool sta_succ = initialise_sta_wifi(PARAM_ESP_WIFI_SSID);
    if (!sta_succ) { 
        //restart sequential wifi
        kill_wifi();
        demo_sequential_wifi(PARAM_ESP_WIFI_SSID,PARAM_SSID_LEN);
    }
}





void app_main(void)
{
	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();
    //init_uart();

    const int UNIQUE_ID_LENGTH = 16;
    char chipid[UNIQUE_ID_LENGTH];

    get_chip_id(chipid, UNIQUE_ID_LENGTH);

    //Start Wifi
	//initialise_sta_wifi();
    //initialize_softAP_wifi(chipid, UNIQUE_ID_LENGTH);
    //example_connect()
    demo_sequential_wifi(chipid, UNIQUE_ID_LENGTH); //this returns on successful connection
    cv_mqtt_init(chipid, UNIQUE_ID_LENGTH, desired_mqtt_broker_ip);
}




