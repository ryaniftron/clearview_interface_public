#ifndef CV_SERVER_C
#define CV_SERVER_C

#include <esp_event.h>
#include <esp_log.h>

#include "lwip/api.h"

#include "cv_utils.h"
#include "cv_mqtt.h"
#include "cv_uart.h"
#include "cv_ota.h"
#include "cv_ledc.h"
#include "cv_api.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b)) //where does this come from? see http_server_simple example
#endif //MIN

const char* TAG_SERVER = "CV_SERVER";
static bool _server_started = false;


#define HTML_ROOT_TITLE \
"<h2><center>ClearView Wireless %s  </center></h2>" //supply chip_id

#define HTML_ROOT_SUBTITLE \
"<table style=\"table-layout: fixed; width: 100\%%;\">\
  <tr>\
    <td style=\"text-align:left; width:50\%%\">Seat: %d</td>\
    <td style=\"text-align:left;  width:50\%%\"> Lock: %s</td>\
  </tr>\
</table>\
<hr>"

// '/' , the root text configuration
#define HTML_WIFICONFIG \
    "<form action=\"/config_wifi\" method> \
        <label for=\"ssid\">Network SSID:</label><br> \
        <input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"%s\"><br> \
        <label for=\"password\">Network Password:</label><br> \
        <input type=\"text\" id=\"password\" name=\"password\" value=\"%s\"><br> \
        <label for=\"device_name\">Device Name:</label><br> \
        <input type=\"text\" id=\"device_name\" name=\"device_name\" value=\"%s\"><br> \
        <label for=\"broker_ip\">Broker IP Address:</label><br> \
        <input type=\"text\" id=\"broker_ip\" name=\"broker_ip\" value=\"%s\"><br> \
        <input type=\"submit\" value=\"Save and Join Network\">\
    "
extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[]   asm("_binary_settings_html_end");

extern const uint8_t sxstart[] asm("_binary_test_txt_start");
extern const uint8_t sxend[] asm("_binary_test_txt_end");

extern const uint8_t i2start[] asm("_binary_index2_html_start");
extern const uint8_t i2end[] asm("_binary_index2_html_end");



// '/settings' body
#define HTML_SETTINGS \
    "<form method=\"POST\"> \
    <label for=\"nodeN\">Seat Number:</label>\
    <select id=\"nodeN\" name = \"node_number\">\
        <option value=\"0\">1</option>\
        <option value=\"1\">2</option>\
        <option value=\"2\">3</option>\
        <option value=\"3\">4</option>\
        <option value=\"4\">5</option>\
        <option value=\"5\">6</option>\
        <option value=\"6\">7</option>\
        <option value=\"7\">8</option>\
    </select>\
    <input type=\"submit\" value=\"Set Seat Number\">\
    </form>\
    <form method=\"POST\"> \
    <label for=\"channel\">Channel:</label>\
    <select id=\"channel\" name = \"channel\">\
        <option value=\"0\">1</option>\
        <option value=\"1\">2</option>\
        <option value=\"2\">3</option>\
        <option value=\"3\">4</option>\
        <option value=\"4\">5</option>\
        <option value=\"5\">6</option>\
        <option value=\"6\">7</option>\
        <option value=\"7\">8</option>\
    </select>\
    <input type=\"submit\" value=\"Set Channel\">\
    </form>\
    <form method=\"POST\"> \
    <label for=\"band\">Band:</label>\
    <select id=\"band\" name = \"bandz\">\
        <option value=\"0\">1</option>\
        <option value=\"1\">2</option>\
        <option value=\"2\">3</option>\
        <option value=\"3\">4</option>\
        <option value=\"4\">5</option>\
        <option value=\"5\">6</option>\
        <option value=\"6\">7</option>\
        <option value=\"7\">8</option>\
    </select>\
    <input type=\"submit\" value=\"Set Band\">\
    </form>\
    "

#define HTML_TEST \
"<form action=\"/config_test\">\
  <label for=\"LED\">Choose a LED State</label>\
  <select id=\"led\" name=\"LED\" >\
    <option value=\"ON\">Turn On</option>\
    <option value=\"OFF\">Turn Off</option>\
  </select>\
  <input type=\"submit\" value = \"Set LED State\">\
</form>\
\
<form action=\"/config_test\">\
  <label for=\"user_message\">Enter User Message</label>\
  <input type=\"text\" id=\"user_message\" name=\"UM\">\
  <input type=\"submit\" value = \"Set User Message\">\
</form>\
\
<form action=\"/config_test\">\
  <input type=\"text\" id=\"req_lock\" name=\"RL\">\
  <input type=\"submit\" value = \"Request Lock State\">\
  %s\
</form>\
"
 

// URI Defines
#define CV_CONFIG_WIFI_URI "/config_wifi"
#define CV_CONFIG_WIFI_INSTANT_URI "/config_wifi_instant"

//TODO make these a struct
bool ssid_ready = false;
bool password_ready = false;
bool device_name_ready = false;
bool broker_ip_ready = false;

static esp_err_t config_test_get_handler(httpd_req_t *req)
{
//Parse the URL QUERY
    char*  buf;
    size_t buf_len;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Found config_test URL query => %s", buf);
            char param[32];
            // /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "UM", param, sizeof(param)) == ESP_OK) {
                ESP_LOGD(TAG_SERVER, "Found URL query parameter => UM=%s", param);
                size_t needed = snprintf(NULL, 0, "\00209UM%s%%\003",param)+1;
                char* line = (char*)malloc(needed);
                snprintf(line, needed,  "\00209UM%s%%\003",param);
                if (cvuart_send_command(line)){
                    remove_ctrlchars(line);
                    httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN);
                } else {
                    httpd_resp_send_chunk(req, "Error: uart disabled", HTTPD_RESP_USE_STRLEN);
                }

            }
            if (httpd_query_key_value(buf, "RL", param, sizeof(param)) == ESP_OK) {
                ESP_LOGD(TAG_SERVER, "Found URL query parameter => Request Lock Status"); //unused param
                uint8_t* dataRx = (uint8_t*) malloc(RX_BUF_SIZE+1);
                int repCount = cvuart_send_report(LOCK_REPORT_FMT, dataRx);
                if (repCount > 0){
                    remove_ctrlchars((char*)dataRx);
                    httpd_resp_send_chunk(req, (char*)dataRx, HTTPD_RESP_USE_STRLEN);
                } else if (repCount == 0 ) {
                    httpd_resp_send_chunk(req, "No uart response", HTTPD_RESP_USE_STRLEN);
                } else if (repCount == -1) {
                    httpd_resp_send_chunk(req, "Error: uart disabled", HTTPD_RESP_USE_STRLEN);
                }
                    // cvuart_send_report("\n09RPMV%%\r", dataRx);
                    // cvuart_send_report("\n09RPVF%%\r", dataRx);
                    //cvuart_send_report("\n09RPID%%\r", dataRx);
                    //run_cv_uart_test_task();
                free(dataRx);
            }
            if (httpd_query_key_value(buf, "LED", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => LED=%s", param);
                #if CONFIG_ENABLE_LED
                    if ((strncmp(param, "ON", strlen(param))) == 0){
                        httpd_resp_send_chunk(req, "LED ON", HTTPD_RESP_USE_STRLEN);
                        set_ledc_code(0, led_on);
                    } else if ((strncmp(param, "OFF", strlen(param))) == 0){
                        httpd_resp_send_chunk(req, "LED OFF", HTTPD_RESP_USE_STRLEN);;
                        set_ledc_code(0, led_off);
                    } else {
                        ESP_LOGE(TAG_SERVER, "Unsupported LED value of %s", param);
                    }
                #else //not CONFIG_ENABLE_LED
                    httpd_resp_send_chunk(req, "Error: LED is disabled", HTTPD_RESP_USE_STRLEN);
                #endif //CONFIG_ENABLE_LED
            }    
        }
        free(buf);
    }

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static esp_err_t config_settings_get_handler(httpd_req_t *req)
{
    // char*  buf;
    // size_t buf_len;
    //
    // /* Read URL query string length and allocate memory for length + 1,
    //  * extra byte for null termination */
    // buf_len = httpd_req_get_url_query_len(req) + 1;
    // if (buf_len > 1) {
    //     buf = malloc(buf_len);
    //     if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    //         ESP_LOGI(TAG_SERVER, "Found URL query => %s", buf);
    //         char param[32];
    //         /* Get value of expected key from query string */
    //         if (httpd_query_key_value(buf, "node_number", param, sizeof(param)) == ESP_OK) {
    //             ESP_LOGI(TAG_SERVER, "Found URL query parameter => node_number=%s", param);
    //             if (set_credential("node_number", param)){
    //                 update_subscriptions_new_node();
    //             }
    //         }
    //     }
    //     free(buf);
    //     httpd_resp_send_chunk(req, "Settings Accepted", HTTPD_RESP_USE_STRLEN);    
    //     httpd_resp_send_chunk(req, NULL, 0);
    // }
    // return ESP_OK;

    char buf[100]; //Todo check expected behavior for buffer overflow
    int ret, remaining = req->content_len;

    
    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        char val[64];
        if (httpd_query_key_value(buf, "address", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting address to: %s", val);
            set_address(val);
        } 
        else if (httpd_query_key_value(buf, "antenna_mode", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting antenna mode to: %s", val);
            set_antenna(val);
        } 
        else if (httpd_query_key_value(buf, "channel", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting channel to: %s", val);
            set_channel(val);
        } 
        else if (httpd_query_key_value(buf, "band", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting band to: %s", val);
            set_band(val);
        } 
        else if (httpd_query_key_value(buf, "id", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting user id string to: %s", val);
            set_id(val);
        } 
        else if (httpd_query_key_value(buf, "user_message", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting user message string to: %s", val);
            set_usermsg(val);
        } 
        else if (httpd_query_key_value(buf, "mode", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting mode to: %s", val);
            set_mode(val);
        } 
        else if (httpd_query_key_value(buf, "osd_visibility", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting osd visibility to: %s", val);
            set_osdvis(val);
        } 
        else if (httpd_query_key_value(buf, "osd_position", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting osd position to: %s", val);
            set_osdpos(val);
        }
        else if (httpd_query_key_value(buf, "reset_lock", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Resetting Lock; unused val: %s", val);
            reset_lock();
        } 
        else if (httpd_query_key_value(buf, "video_format", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting video format to: %s", val);
            set_videoformat(val);
        }
        else if (httpd_query_key_value(buf, "seat_number", val, sizeof(val)) == ESP_OK) {
            ESP_LOGD(TAG_SERVER, "Setting seat number to: %s", val);
            if (set_credential("node_number", val)){
                update_subscriptions_new_node();
            }
        } 
        else {
            ESP_LOGW(TAG_SERVER, "Unkown parm in %s", buf);
            httpd_resp_set_status(req, HTTPD_400);
            httpd_resp_send_chunk(req, "Error: Invalid API Endpoint<br>", HTTPD_RESP_USE_STRLEN);
            httpd_resp_send_chunk(req, buf, ret);
            httpd_resp_send_chunk(req, NULL, 0);
            return ESP_OK;
        }

        //Redirect
        httpd_resp_send_chunk(req, "<head>", HTTPD_RESP_USE_STRLEN); 
        char* redirect_str = "<meta http-equiv=\"Refresh\" content=\"3; URL=http://192.168.4.1/settings\">";
        httpd_resp_send_chunk(req, redirect_str, HTTPD_RESP_USE_STRLEN);
        httpd_resp_send_chunk(req, "</head>", HTTPD_RESP_USE_STRLEN);  
        httpd_resp_send_chunk(req, "Redirecting in 3s. TODO verify data was set in CV <br>", HTTPD_RESP_USE_STRLEN);

        

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG_SERVER, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG_SERVER, "%.*s", ret, buf);
        ESP_LOGI(TAG_SERVER, "====================================");
    }
   
               

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG_SERVER, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG_SERVER, "%.*s", ret, buf);
        ESP_LOGI(TAG_SERVER, "====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t config_wifi_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;


    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_SERVER, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => ssid=%s", param);
                ssid_ready = true; //TODO state machine set false
                set_credential("ssid", param);
            }
            if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => password=%s", param);
                password_ready = true;
                set_credential("password", param);
            }
            if (httpd_query_key_value(buf, "device_name", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => device_name=%s", param);
                device_name_ready = true;
                set_credential("device_name", param);
            }
            if (httpd_query_key_value(buf, "broker_ip", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => broker_ip=%s", param);
                broker_ip_ready = true;
                set_credential("broker_ip", param);
            }
        }
        free(buf);

        if (ssid_ready && password_ready && device_name_ready && broker_ip_ready){
            

            //Match the URI to determine whether to send instantly or not
            printf("%s\n", req->uri);
            bool is_instant = false;
            bool is_matched = false;

            bool uri_match;
            uri_match = httpd_uri_match_wildcard(CV_CONFIG_WIFI_URI, req->uri, strlen(CV_CONFIG_WIFI_URI));
            if (uri_match) {
                is_instant = false;
                is_matched = true;
            }
            uri_match = httpd_uri_match_wildcard(CV_CONFIG_WIFI_INSTANT_URI, req->uri, strlen(CV_CONFIG_WIFI_INSTANT_URI));;
            if (uri_match) {
                            is_instant = false;
                            is_matched = true;
            }
            if (!is_matched){
                //ESP_LOGI(TAG_SERVER, LOG_FMT("URI '%s' unmatched"), req->uri);
                ESP_LOGE(TAG_SERVER, "Unmatched URI");
            }

            /*
            // TODO add in content in the response to say "form submitted".
            // Redirect both address immediately to "/wifi_config_saved". Save _is_instant. 
            // Set up /wifi_config_saved to serve the saved wifi settings. 
            // Try encoding the data as a json file encoding using the HTML headers */

            if (is_instant){
                ESP_LOGI(TAG_SERVER, "All WiFi Creds received. Configuring now");
                // HTML Redirect https://developer.mozilla.org/en-US/docs/Web/HTTP/Redirections
                // httpd_resp_send_chunk(req, "<head>", HTTPD_RESP_USE_STRLEN); 
                // char* redirect_str = "<meta http-equiv=\"Refresh\" content=\"0; URL=http://192.168.4.1/\">";
                // httpd_resp_send_chunk(req, redirect_str, HTTPD_RESP_USE_STRLEN);
                // httpd_resp_send_chunk(req, "</head>", HTTPD_RESP_USE_STRLEN);     
                // httpd_resp_send_chunk(req, NULL, 0);
                // vTaskDelay(1000 / portTICK_PERIOD_MS);
            } else {
                ESP_LOGI(TAG_SERVER, "All WiFi Creds received. Sending Delayed Redirect");
                // httpd_resp_send_chunk(req, "<head>", HTTPD_RESP_USE_STRLEN); 
                // char* redirect_str = "<meta http-equiv=\"Refresh\" content=\"3; URL=http://192.168.4.1/\">";
                // httpd_resp_send_chunk(req, redirect_str, HTTPD_RESP_USE_STRLEN);
                // httpd_resp_send_chunk(req, "</head>", HTTPD_RESP_USE_STRLEN);     
                // httpd_resp_send_chunk(req, NULL, 0);
                //vTaskDelay(6000 / portTICK_PERIOD_MS);
            }
            
            // TODO the redirect may be causing an issue as far as timing and switching out of the ode
            // Maybe this should return with no delay? 
            /* 
            W (164882) httpd_txrx: httpd_sock_err: error in send : 113
            ESP_ERROR_CHECK failed: esp_err_t 0xb006 (ERROR) at 0x40090df4
            0x40090df4: _esp_error_check_failed at /home/ryan/esp/esp-idf/components/esp32/panic.c:726

            file: "../main/cv_server.c" line 208
            func: serve_title
            expression: httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN)
            */
            
            // HTTP Redirect Method
            // https://esp32.com/viewtopic.php?t=11012
            // httpd_resp_set_type(req, "text/html");
            // httpd_resp_set_status(req, "307 Temporary Redirect");
            // httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/wifi_confirmation");
            httpd_resp_send(req, NULL, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            extern bool switch_to_sta;
            switch_to_sta = true;
        }
    }

    return ESP_OK;
}

void serve_title(httpd_req_t *req) {
    const int UNIQUE_ID_LENGTH = 16;
    char chipid[UNIQUE_ID_LENGTH];
    get_chip_id(chipid, UNIQUE_ID_LENGTH);

    size_t needed = snprintf(NULL, 0, HTML_ROOT_TITLE, chipid)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, HTML_ROOT_TITLE, chipid);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
}

void serve_node_and_lock(httpd_req_t *req){
    extern uint8_t desired_node_number;
    char* lock_status = "TODO";

    // Display the node number 1 higher than stored in the back end. 
    // Back end
    int ui_node_number = desired_node_number+1;
    size_t needed = snprintf(NULL, 0, HTML_ROOT_SUBTITLE, ui_node_number, lock_status)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, HTML_ROOT_SUBTITLE, ui_node_number, lock_status);
    //ESP_LOGI(TAG_SERVER, "Node and Lock Request %d ,%s", node_number, "TODO");
    // size_t needed = snprintf(NULL, 0, HTML_ROOT_SUBTITLE, node_number, lock_status);
    
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
    //ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
}

void serve_wificonfig(httpd_req_t *req){
    size_t needed = snprintf(NULL, 0, HTML_WIFICONFIG, desired_ap_ssid, desired_ap_pass, desired_friendly_name, desired_mqtt_broker_ip)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, HTML_WIFICONFIG, desired_ap_ssid, desired_ap_pass, desired_friendly_name, desired_mqtt_broker_ip);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
}

void serve_settingsconfig(httpd_req_t *req){
    //     char buf[100];
    // char buf2[100];
    // // printf("Length %d\n", sxend-sxstart);
    // printf("Length i2 %d\n", i2end- i2start);
    // uint8_t i;
    // // //snprintf(buf, sxend-sxstart, "%s", sxstart);
    // snprintf(buf2, i2end-i2start, "%s\n", i2start );
    // printf("TESTX: %s\n", buf2);
    // ESP_ERROR_CHECK(httpd_resp_send_chunk(req, HTML_SETTINGS, HTTPD_RESP_USE_STRLEN));
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)i2start, i2end - i2start));
}

void serve_test(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, HTML_TEST, HTTPD_RESP_USE_STRLEN));
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    serve_title(req);
    serve_node_and_lock(req);
    serve_wificonfig(req);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, NULL, 0));
    return ESP_OK;
}


static esp_err_t settings_get_handler(httpd_req_t *req)
{
    serve_title(req);
    serve_node_and_lock(req);
    serve_settingsconfig(req);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, NULL, 0));
    return ESP_OK;
}

static esp_err_t hello_cv_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_SERVER, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => ssid=%s", param);
            }
            if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => password=%s", param);
            }
            if (httpd_query_key_value(buf, "device_name", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => device_name=%s", param);
            }
            if (httpd_query_key_value(buf, "broker_ip", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => broker_ip=%s", param);
            }           
        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    //Original method: One response
    // httpd_resp_send(req, resp_str, strlen(resp_str));

    //New Method: Chunks
    httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN); //HTTPD_RESP_USE_STRLEN

    //Now, send a second string
    char* lock_info = "Lock Status: %i";
    int lock_status = 23;
    size_t needed = snprintf(NULL, 0,lock_info, lock_status);
    char* next_str = (char*)malloc(needed);
    snprintf(next_str, needed,lock_info);
    httpd_resp_send_chunk(req, next_str,HTTPD_RESP_USE_STRLEN);
    free(next_str);
    
    httpd_resp_send_chunk(req, NULL, 0);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_SERVER, "Request headers lost");
    }
    return ESP_OK;
}


static esp_err_t test_cv_get_handler(httpd_req_t *req)
{
    serve_title(req);
    serve_test(req);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, NULL, 0));
    return ESP_OK;
}

static const httpd_uri_t echo_uri = {
    .uri = "/post_test",
    .method = HTTP_POST,
    .handler= echo_post_handler
};

static const httpd_uri_t config_settings_uri = {
    .uri       = "/settings",
    .method    = HTTP_POST,
    .handler   = config_settings_get_handler
};

// When a user manually submits wifi config. Issues a confirmation page
static const httpd_uri_t config_wifi_uri = {
    .uri       = CV_CONFIG_WIFI_URI,
    .method    = HTTP_GET,
    .handler   = config_wifi_get_handler
};

// Used to submit wifi credentials without promting or redirecting
static const httpd_uri_t config_wifi_instant_uri = {
    .uri   = CV_CONFIG_WIFI_INSTANT_URI,
    .method    = HTTP_GET,
    .handler   = config_wifi_get_handler
};

static const httpd_uri_t settings_uri = {
    .uri       = "/settings",
    .method    = HTTP_GET,
    .handler   = settings_get_handler
};

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t hello_uri = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_cv_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World"
};

#ifdef CONFIG_CV_INITIAL_PROGRAM
static const httpd_uri_t test_uri = {
    .uri       = "/test",
    .method    = HTTP_GET,
    .handler   = test_cv_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
};

static const httpd_uri_t config_test_uri = {
    .uri       = "/config_test",
    .method    = HTTP_GET,
    .handler   = config_test_get_handler
};
#endif // CONFIG_CV_INITIAL_PROGRAM



extern httpd_handle_t start_cv_webserver(void){
    if (_server_started) {
        ESP_LOGW(TAG_SERVER, "SERVER is already running");
        return NULL;
    }


    esp_log_level_set(TAG_SERVER, ESP_LOG_DEBUG);
    //start the OTA reboot task
    xTaskCreate(&otaRebootTask, "rebootTask", 2048, NULL, 5, NULL);

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Lets bump up the stack size (default was 4096)
	config.stack_size = 8192;
    config.max_uri_handlers = 14;
    ESP_LOGI(TAG_SERVER, "Starting server on port: '%d'", config.server_port);
    esp_err_t ret = httpd_start(&server, &config);
    ESP_ERROR_CHECK(ret); // this should bail if the server can't start
    // Start the httpd server
    ESP_LOGI(TAG_SERVER, "Server started");
    
    if (ret == ESP_OK){
        // Set URI handlers
        ESP_LOGI(TAG_SERVER, "Registering URI handlers");
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &hello_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_wifi_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_wifi_instant_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_settings_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &settings_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_index_html));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_favicon_ico));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_jquery_3_4_1_min_js));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_update));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_status));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &echo_uri));

        #ifdef CONFIG_CV_INITIAL_PROGRAM
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &test_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_test_uri));
        #endif

        ESP_LOGI(TAG_SERVER, "Webserver started. Login and go to http://192.168.4.1/ \n");
        _server_started = true;
        return server;
    }

    ESP_LOGI(TAG_SERVER, "Error starting server!");
    _server_started = false;
    return NULL;
}


static void stop_cv_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
    _server_started = false;
}

//TODO link the disconnect and connect handlers to joinging/leaving the wifi?

static void cv_disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG_SERVER, "Stopping webserver");
        stop_cv_webserver(*server);
        *server = NULL;
    }
}

static void cv_connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG_SERVER, "Starting webserver");
        *server = start_cv_webserver();
    }
}

#endif //CV_SERVER_C