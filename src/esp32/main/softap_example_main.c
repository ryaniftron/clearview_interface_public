#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

#include "lwip/err.h"
#include "string.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

#define AP_TARGET_SSID "myssid"
#define AP_TARGET_PASSWORD "mypassword"

/* if code below is uncommented static ip is used */ 
/*
#define DEVICE_IP          "192.168.178.4"
#define DEVICE_GW          "192.168.178.1"
#define DEVICE_NETMASK     "255.255.255.0"
*/

#define HDR_200 "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n"
#define HDR_201 "HTTP/1.1 201 Created\r\nContent-type: text/html\r\n\r\n"
#define HDR_204 "HTTP/1.1 204 No Content\r\nContent-type: text/html\r\n\r\n"
#define HDR_404 "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n"
#define HDR_405 "HTTP/1.1 405 Method not allowed\r\nContent-type: text/html\r\n\r\n"
#define HDR_409 "HTTP/1.1 409 Conflict\r\nContent-type: text/html\r\n\r\n"
#define HDR_501 "HTTP/1.1 501 Not Implemented\r\nContent-type: text/html\r\n\r\n"
//#define root_text "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br> "


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
        <input type=\"submit\" value=\"Save and Join Network\">\
    </form> \
            \
    <form action=\"/scan_wifi\" method> \
        <input type=\"submit\" value=\"Scan for WiFi\">\
    </form> \
"

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
    " Attempting wifi connection... TODO timeout <br> \
    "
    
#define config_esp32_parse_fail \
    " Error: config_esp32 Parse Fail <br> \
    "


void set_credential(char* credentialName, char* val){
    printf("##Setting %s = %s##", credentialName, val);
    // TODO store these values inside ESP32 then join access point
}


// parse the request payload for credentials for the wifi device
// Successful parse returns true
bool parse_net_credentials(char* payload){
    ///example: /config_esp32?ssid=a&password=x&device_name=z
    printf("Parsing these creds: %s\n", payload);

    char credentialSearchTerms[4][15] = {"ssid","password","device_name","\0"};



    char * start,*stop,*arg1, *k, *knext; 
    
    int nConfTerms = 3;
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
    return true;
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


static void initialise_wifi(void)
{
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

#ifdef DEVICE_IP
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
	tcpip_adapter_ip_info_t ipInfo;
	inet_pton(AF_INET, DEVICE_IP, &ipInfo.ip);
	inet_pton(AF_INET, DEVICE_GW, &ipInfo.gw);
	inet_pton(AF_INET, DEVICE_NETMASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
#endif

	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    //TODO: Set up both station and AP mode:
    //  https://www.esp32.com/viewtopic.php?t=10619


	wifi_config_t sta_config = {
		.sta = {
			.ssid = AP_TARGET_SSID,
			.password = AP_TARGET_PASSWORD,
			.bssid_set = false
		}
	};

  ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
  ESP_ERROR_CHECK( esp_wifi_start() );
}

static void
http_server_netconn_serve(struct netconn *conn)
{
	struct netbuf *inbuf;
	char *buf, *payload, *start, *stop;
	u16_t buflen;
	err_t err;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
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

		/* For now  only GET results in a valid respons */
		if (strncmp(buf, "GET /", 5) == 0){
            netconn_write(conn, HDR_200, sizeof(HDR_200)-1, NETCONN_NOCOPY);
			//printf("GET = '%s' \n", payload);
            if (strcmp(payload, "/") == 0 || strcmp(payload, "/?") == 0) { //give the root website
                netconn_write(conn, root_text, sizeof(root_text)-1, NETCONN_NOCOPY);
            }else if (strcmp(payload, "/favicon.ico") == 0) {
                //do nothing
            }else if (strcmp(payload, "/scan_wifi?") == 0) {
                netconn_write(conn, wifi_scan_text, sizeof(wifi_scan_text)-1, NETCONN_NOCOPY);
            }else if (strncmp(payload, "/config_esp32", strlen("/config_esp32")) == 0){
                printf("Got a config filestring\n");
                //GET='GET /config_esp32?ssid=A&password=B&device_name=C '
                if (parse_net_credentials(payload)){
                    netconn_write(conn, wifi_attempt_connect, sizeof(wifi_attempt_connect)-1, NETCONN_NOCOPY);
                }else {
                    netconn_write(conn, config_esp32_parse_fail, sizeof(config_esp32_parse_fail)-1, NETCONN_NOCOPY);
                }
            }else{  
                //char* errmsg = sprintf("Uncaptured GET='%s' \n", *payload);
                //printf("Uncaptured GET='%s' \n", &payload);
                //netconn_write(conn, errmsg, sizeof(errmsg)-1, NETCONN_NOCOPY);              
            }

            
            /* send "hello world to client" */
            
			
		}else if (strncmp(buf, "POST /", 6) == 0){
			/* send '501 Not implementd' reply  */
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "PUT /", 5) == 0){
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "PATCH /", 7) == 0){
			/* send '501 Not implementd' reply  */
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else if (strncmp(buf, "DELETE /", 8) == 0){
			/* send '501 Not implementd' reply  */
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}else{
			/* 	Any unrecognized verb will automatically 
				result in '501 Not implementd' reply */
			netconn_write(conn, HDR_501, sizeof(HDR_501)-1, NETCONN_NOCOPY);
		}
		free(payload);
	}
  	/* Close the connection (server closes in HTTP) and clean up after ourself */
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


int app_main(void)
{
    //uint64_t *mac = ESP_MAC_WIFI_STA;
    //esp_efuse_mac_get_default(mac);
    //printf("%s",mac);

	nvs_flash_init();
	initialise_wifi();

	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
	return 0;
}
