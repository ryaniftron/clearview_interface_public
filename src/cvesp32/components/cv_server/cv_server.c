#ifndef CV_SERVER_C
#define CV_SERVER_C

#define WRITE_THEN_READ 1


#include <esp_event.h>
#include <esp_log.h>
#include <cJSON.h>
#include <cJSON_Utils.h>


#include "lwip/api.h"

#include "cv_utils.h"
#include "cv_mqtt.h"
#include "cv_uart.h"
#include "cv_ota.h"
#include "cv_ledc.h"
#include "cv_api.h"
#include "cv_utils.h"
#include "cv_wifi.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b)) //where does this come from? see http_server_simple example
#endif //MIN

#define POST_BUFFER_SZ 400

const char* TAG_SERVER = "CV_SERVER";
static bool _server_started = false;

extern const uint8_t title_html_start[] asm("_binary_title_html_start");
extern const uint8_t title_html_end[]   asm("_binary_title_html_end");

extern const uint8_t subtitle_html_start[] asm("_binary_subtitle_html_start");
extern const uint8_t subtitle_html_end[]   asm("_binary_subtitle_html_end");

extern const uint8_t wifiSettings_html_start[] asm("_binary_wifiSettings_html_start");
extern const uint8_t wifiSettings_html_end[]   asm("_binary_wifiSettings_html_end");

extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[]   asm("_binary_settings_html_end");

extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[]   asm("_binary_settings_html_end");

extern const uint8_t test_html_start[] asm("_binary_test_html_start");
extern const uint8_t test_html_end[]   asm("_binary_test_html_end");

extern const uint8_t html_beg_start[] asm("_binary_httpDocBegin_html_start");
extern const uint8_t html_beg_end[] asm("_binary_httpDocBegin_html_end");

extern const uint8_t html_conc_start[] asm("_binary_httpDocConclude_html_start");
extern const uint8_t html_conc_end[] asm("_binary_httpDocConclude_html_end");

extern const uint8_t cv_js_start[] asm("_binary_cv_js_js_start");
extern const uint8_t cv_js_end[] asm("_binary_cv_js_js_end");

#ifdef CONFIG_CV_INITIAL_PROGRAM
extern const uint8_t menu_bar_start[] asm("_binary_menuBarTest_html_start");
extern const uint8_t menu_bar_end[] asm("_binary_menuBarTest_html_end");
#else
extern const uint8_t menu_bar_start[] asm("_binary_menuBar_html_start");
extern const uint8_t menu_bar_end[] asm("_binary_menuBar_html_end");
#endif


// API ENDPOINTS
#define CV_API_ADDRESS "address"
#define CV_API_SSID "ssid"
#define CV_API_PASSWORD "password"
#define CV_API_DEVICE_NAME "device_name"
#define CV_API_BROKER_IP "broker_ip"
#define CV_API_SEAT "seat"
#define CV_API_ANTENNA "antenna"
#define CV_API_CHANNEL "channel"
#define CV_API_BAND "band"
#define CV_API_ID "id"
#define CV_API_USER_MESSAGE "user_msg"
#define CV_API_MODE "mode"
#define CV_API_OSD_VISIBILITY "osd_visibility"
#define CV_API_OSD_POSITION "osd_position"
#define CV_API_LOCK "lock"
#define CV_API_VIDEO_FORMAT "video_format"
#define CV_API_CV_VERSION "cv_version"
#define CV_API_CVCM_VERSION "cvcm_version"
#define CV_API_CVCM_VERSION_ALL "cvcm_version_all"
#define CV_API_LED "led"
#define CV_API_SEND_CMD "send_cmd"
#define CV_API_REQ_REPORT "req_report"
#define CV_API_MAC_ADDR "mac_addr"
#define CV_API_WIFI_STATE "wifi_state"
#define CV_API_WIFI_POWER "wifi_power"

#define CV_READ_Q "?"

// //TODO Deprecated
// bool ssid_ready = false;
// bool password_ready = false;
// bool device_name_ready = false;
// bool broker_ip_ready = false;

//Return 0 if content type is json
static int is_content_json(httpd_req_t *req){
    //Check content type
    const char CSZ = 50;
    char content_type[CSZ];
    httpd_req_get_hdr_value_str(req, "Content-Type", content_type, CSZ);
    return strncmp(HTTPD_TYPE_JSON, content_type, strlen(content_type));
}

static int is_content_html_plain(httpd_req_t *req){
    //Check content type
    const char CSZ = 50;
    char content_type[CSZ];
    httpd_req_get_hdr_value_str(req, "Content-Type", content_type, CSZ);
    return strncmp(HTTPD_TYPE_TEXT, content_type, strlen(content_type));
}

void add_response_to_json_car(cJSON* ret, char* k, struct cv_api_read* car){
    if (car->success) {
        cJSON_AddStringToObject(ret, k, car->val);
    }
    else {
        if (car->api_code == CV_ERROR_NO_COMMS) {cJSON_AddStringToObject(ret, k, "error-no_comms");}
        else if (car->api_code == CV_ERROR_WRITE) {cJSON_AddStringToObject(ret, k, "error-write_on_read_only_param");} 
        else if (car->api_code == CV_ERROR_READ) {cJSON_AddStringToObject(ret, k, "error-read_on_write_only_param");} 
        else if (car->api_code == CV_ERROR_VALUE) {cJSON_AddStringToObject(ret, k, "error-value");} 
        else if (car->api_code == CV_ERROR_INVALID_ENDPOINT) {cJSON_AddStringToObject(ret, k, "error-invalid_endpoint");}
        else if (car->api_code == CV_ERROR_NVS_WRITE) {cJSON_AddStringToObject(ret, k, "error-nvs_write");}
        else if (car->api_code == CV_ERROR_NVS_READ) {cJSON_AddStringToObject(ret, k, "error-nvs_read");}
        else {CV_LOGE("artjc", "Unknown car->api_code");}
    }
    //if (car->val != NULL) free(car->val); //TODO memory needs freeing
}

void add_response_to_json_caw(cJSON* ret, char* k, struct cv_api_write* caw, char* v) {
    if (caw->success) {
        cJSON_AddStringToObject(ret, k, v);
    }
    else {
        if (caw->api_code == CV_ERROR_NO_COMMS) {cJSON_AddStringToObject(ret, k, "error-no_comms");}
        else if (caw->api_code == CV_ERROR_WRITE) {cJSON_AddStringToObject(ret, k, "error-write");} 
        else if (caw->api_code == CV_ERROR_READ) {cJSON_AddStringToObject(ret, k, "error-read");} 
        else if (caw->api_code == CV_ERROR_VALUE) {cJSON_AddStringToObject(ret, k, "error-value");} 
        else if (caw->api_code == CV_ERROR_INVALID_ENDPOINT) {cJSON_AddStringToObject(ret, k, "error-invalid_endpoint");}
        else if (caw->api_code == CV_ERROR_NVS_WRITE) {cJSON_AddStringToObject(ret, k, "error-nvs_write");}
        else if (caw->api_code == CV_ERROR_NVS_READ) {cJSON_AddStringToObject(ret, k, "error-nvs_read");}
        else {CV_LOGE("artjc", "Unknown caw->api_code");}
    }
}

// Parse read request, execute it, and fill the data in car
void kv_api_parse_car(struct cv_api_read* car, char* k, char* v) {
    char* TAG = "cv_server->kv_api_parse_car";
    CV_LOGI(TAG, "Getting value for '%s'",k);
        
    if (strncmp(k, CV_API_CHANNEL, strlen(k)) == 0){
        get_channel(car);
    }else if (strncmp(k, CV_API_BAND, strlen(k)) == 0){
        get_band(car);
    }else if (strncmp(k, CV_API_ID, strlen(k)) == 0){
        get_id(car);
    }else if (strncmp(k, CV_API_REQ_REPORT, strlen(k)) == 0){
        get_custom_report(v, car);
    }else if (strncmp(k, CV_API_SEAT, strlen(k)) == 0){
        if (get_nvs_value(nvs_seat_number)){
            car->success = true;
            // sprintf(car->val, "%d", desired_seat_number);
            //car->val = strdup(desired_seat_number);
            char sn[2];
            itoa(desired_seat_number, sn, 10);
            car->val = strdup(sn);
            car->api_code = CV_OK;
        } else {
            car->success = false;
            car->api_code = CV_ERROR_NVS_READ;
        }
    }else if (strncmp(k, CV_API_SSID, strlen(k)) == 0){
        //get_nvs_value(nvs_wifi_ssid); //Don't read from nvs because it will overright the local value
        car->val = desired_ap_ssid;
        car->api_code = CV_OK;
    }else if (strncmp(k, CV_API_PASSWORD, strlen(k)) == 0){
        //get_nvs_value(nvs_wifi_pass); //Don't read from nvs because it will overright the local value
        car->val = desired_ap_pass;
        car->api_code = CV_OK;
    }else if (strncmp(k, CV_API_DEVICE_NAME, strlen(k)) == 0){
        //get_nvs_value(nvs_...); //Don't read from nvs because it will overright the local value
        car->val = desired_friendly_name;
        car->api_code = CV_OK;
    }else if (strncmp(k, CV_API_BROKER_IP, strlen(k)) == 0){
        //get_nvs_value(nvs_...); //Don't read from nvs because it will overright the local value
        car->val = desired_mqtt_broker_ip;
        car->api_code = CV_OK;
    } else if (strncmp(k, CV_API_CVCM_VERSION, strlen(k)) == 0){
        get_cvcm_version(car);
    } else if (strncmp(k, CV_API_CVCM_VERSION_ALL, strlen(k)) == 0){
        get_cvcm_version_all(car);
    }else if (strncmp(k, CV_API_MAC_ADDR, strlen(k)) == 0){
        get_mac_addr(car);
    }else if (strncmp(k, CV_API_VIDEO_FORMAT, strlen(k)) == 0){
        get_videoformat(car);
    }else if (strncmp(k, CV_API_LED, strlen(k)) == 0){
        get_led(car, 0);//channel 0
    } else if (strncmp(k, CV_API_USER_MESSAGE, strlen(k)) == 0){
        car->success = false;
        car->api_code = CV_ERROR_READ;
    }else if (strncmp(k, CV_API_WIFI_STATE, strlen(k)) == 0){
        CV_WIFI_MODE wifimode = get_wifi_mode();
        ESP_LOGI(TAG_SERVER, "Reading wifi state");
        car->success = true;
        car->api_code=CV_OK;
        if (wifimode == CVWIFI_STA)
            car->val = "sta";
        else if (wifimode == CVWIFI_AP)
            car->val = "ap";
        else {
            car->success = false;
            car->api_code = CV_ERROR_VALUE;
        }
    }else if (strncmp(k, CV_API_WIFI_POWER, strlen(k)) == 0){
        int8_t wifi_power = get_wifi_power();
        ESP_LOGI(TAG_SERVER, "Reading wifi power");
        car->success = true;
        car->api_code = CV_OK;
        char power[6];
        snprintf(power, 6, "%i", wifi_power);
        car->val = strdup(power);
    }else {
        CV_LOGE(TAG,"Unknown request key of '%s'",k );
        car->success = false;
        car->api_code = CV_ERROR_INVALID_ENDPOINT;
    }
}

void caw_nvs_write(struct cv_api_write* caw, char* nvs_key, char* nvs_val){
    caw->success = set_credential(nvs_key, nvs_val);
        if (caw->success){
            caw->api_code = CV_OK;
        } else {
            caw->api_code = CV_ERROR_NVS_WRITE;
        }
}

// Parse command, execute it, and return command or error as JSON
void kv_api_parse_caw(struct cv_api_write* caw, char* k, char* v) {
    char* TAG = "cv_server->kv_api_parse_caw";
    if (strncmp(k, CV_API_ADDRESS, strlen(k)) == 0){
        set_address(v, caw);
    } else if (strncmp(k, CV_API_SSID, strlen(k)) == 0){
        caw_nvs_write(caw, k, v);
    } else if (strncmp(k, CV_API_PASSWORD, strlen(k)) == 0){
        caw_nvs_write(caw, k, v);
    } else if (strncmp(k, CV_API_DEVICE_NAME, strlen(k)) == 0){
        caw_nvs_write(caw, k, v);
    } else if (strncmp(k, CV_API_BROKER_IP, strlen(k)) == 0){
        caw_nvs_write(caw, k, v);
    } else if (strncmp(k, CV_API_SEAT, strlen(k)) == 0){
        caw_nvs_write(caw, k, v);
    } else if (strncmp(k, CV_API_ANTENNA, strlen(k)) == 0){
        set_antenna(v, caw);
    } else if (strncmp(k, CV_API_BAND, strlen(k)) == 0){
        set_band(v, caw);
    } else if (strncmp(k, CV_API_CHANNEL, strlen(k)) == 0){
        set_channel(v, caw);
    } else if (strncmp(k, CV_API_ID, strlen(k)) == 0){
        set_id(v, caw);
    } else if (strncmp(k, CV_API_USER_MESSAGE, strlen(k)) == 0){
        set_usermsg(v, caw);
    } else if (strncmp(k, CV_API_VIDEO_FORMAT, strlen(k)) == 0){
        set_videoformat(v, caw);
    } else if (strncmp(k, CV_API_LED, strlen(k)) == 0){
        set_led(v, caw);
    } else if (strncmp(k, CV_API_SEND_CMD, strlen(k)) == 0) {
        set_custom_w(v, caw);
    } else if (strncmp(k, CV_API_WIFI_STATE, strlen(k))== 0) {
        //TODO put in own function
        CV_WIFI_STATE_MSG state_msg;
        if (strncmp(v,"ap", strlen(v)) == 0){
            state_msg = set_wifi_mode(CVWIFI_AP);
        } else if (strncmp(v, "sta", strlen(v)) == 0){
            state_msg = set_wifi_mode(CVWIFI_STA);
        } else {
            ESP_LOGW(TAG_SERVER, "Invalid wifi value of %s", v);
            caw->success = false;
            caw->api_code = CV_ERROR_VALUE;
            return;
        }
        if (state_msg == CVWIFI_SUCCESS || state_msg == CVWIFI_NO_CHANGE){
            caw->success = true;
            caw->api_code = CV_OK;
        } else {
            caw->success = false;
            caw->api_code = CV_ERROR_VALUE;
        }

    } else if (strncmp(k, CV_API_WIFI_POWER, strlen(k))== 0) {
        if (set_wifi_power_pChr(v)){
            caw->success = true;
            caw->api_code = CV_OK;
        }  else {
            caw->success = false;
            caw->api_code = CV_ERROR_VALUE;
        }
    }else {
        CV_LOGE(TAG,"Unknown write key of '%s'",k );
        caw->success = false;
        caw->api_code = CV_ERROR_INVALID_ENDPOINT;
    }
}


static cJSON* run_kv_api(char* k, char* v){
    //char* TAG = "cv_server->run_kv_api";
    cJSON* ret = cJSON_CreateObject();
    // if it's a request, run request and return
    if (strncmp(v, CV_READ_Q ,strlen(v)) == 0 || strncmp(k,CV_API_REQ_REPORT,strlen(CV_API_REQ_REPORT)) == 0){
        struct cv_api_read car;
        struct cv_api_read*carptr = &car;
        kv_api_parse_car(carptr, k, v);
        add_response_to_json_car(ret, k, carptr);
        vTaskDelay(250 / portTICK_PERIOD_MS); //Delay 250ms after any read
    } else { // It's a command, write value
        struct cv_api_write caw;
        struct cv_api_write* cawptr = &caw;    
        kv_api_parse_caw(cawptr, k, v);
        add_response_to_json_caw(ret, k , cawptr, v);
        vTaskDelay(250 / portTICK_PERIOD_MS); //Delay 250ms between write and read
        #ifdef WRITE_THEN_READ
            // If write success, then do a read and replace the value
            if (cawptr->success){
                struct cv_api_read car;
                struct cv_api_read*carptr = &car;
                vTaskDelay(1000 / portTICK_PERIOD_MS); //Delay 100ms between write and read
                kv_api_parse_car(carptr, k, CV_READ_Q);
                add_response_to_json_car(ret, k, carptr); 
            }
        #endif
    }
    return ret;
}

// Take JSON formatted cmd and return a JSON response
// Supports multiple cJSON elements and will return JSON for each
static cJSON* run_json_api(cJSON* obj)
{
    cJSON* retJSON = NULL; //the return val
    cJSON *element = NULL;
    char *k = NULL;
    char *v = NULL;
    if (cJSON_GetArraySize(obj)){
        cJSON_ArrayForEach(element, obj)
        {
            k = element->string;
            v = element->valuestring;

            if (k != NULL && v != NULL)
            {
                ESP_LOGI("run_api", "key=%s,val=%s", k, v);
                // cJSON_AddItemToObject()
                cJSON* kj = run_kv_api(k, v);
                retJSON = cJSONUtils_MergePatchCaseSensitive(retJSON,kj);
            }
        }
    } else {
        ESP_LOGW(TAG_SERVER, "JSON EMPTY");
    }
    return retJSON;
}


static esp_err_t config_settings_post_handler(httpd_req_t *req)
{
    ESP_LOGD(TAG_SERVER, "Got config_settings request");
        /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char buf[POST_BUFFER_SZ]; //TODO optimize size based on 32char
    
    bool success = true;

    /* Truncate if content length larger than the buffer */
    if (req->content_len >= sizeof(buf)){
        ESP_LOGE(TAG_SERVER, "Content too large. size '%d' > '%d'", req->content_len, sizeof(buf));
        httpd_resp_set_status(req, "413 Payload Too Large");
        httpd_resp_send(req, "Content size exceeded", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }
    size_t recv_size = MIN(req->content_len, sizeof(buf));
    //TODO warn if content too large

    int ret = httpd_req_recv(req, buf, recv_size);
    // printf("\nContent:'%s', ret '%d'\n", buf, ret);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        char* err = "{\"Error=content\":\"0\"}";
        httpd_resp_send_chunk(req,err , HTTPD_RESP_USE_STRLEN);
        httpd_resp_send_chunk(req, NULL, 0);
        ESP_LOGW(TAG_SERVER, "Content Length 0 or other issue");
        
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    if (is_content_json(req) == 0) { //Content is json
        ESP_LOGD(TAG_SERVER, "Content is json");
        // TODO parse JSON here
                // TODO parse JSON here
        cJSON *json_post = cJSON_Parse(buf);
        if (json_post == NULL) {
            ESP_LOGE(TAG_SERVER, "JSON Parse Failure");
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL)
            {
                ESP_LOGI(TAG_SERVER, "JSON Parse Error starting at: %s\n", error_ptr);
                ESP_LOGI(TAG_SERVER, "Full JSON: %s", buf);
            }
            httpd_resp_send_chunk(req, "{\"error\":\"error-json-parse\"}",HTTPD_RESP_USE_STRLEN);
            success = false;
        } else {
            char* cjson_arrived = cJSON_Print(json_post);
            
            printf("JSON In: %s\n", cjson_arrived);
            cJSON *json_returned = run_json_api(json_post); //TODO memory leak?
            if (json_returned){
                char* cjson_returned = cJSON_Print(json_returned);
                printf("JSON Back: %s\n", cjson_returned);
                ESP_ERROR_CHECK(httpd_resp_set_hdr(req,"Content-Type",HTTPD_TYPE_JSON));
                httpd_resp_send_chunk(req, cjson_returned, HTTPD_RESP_USE_STRLEN);  
                free(cjson_returned);
            }
            free(cjson_arrived);
        }
        cJSON_Delete(json_post);
    } else {
        ESP_LOGW(TAG_SERVER, "Content not json - ignore parsing");
        ESP_LOGI(TAG_SERVER, "buf:'%s'",buf);
        httpd_resp_set_status(req, "406 Not Acceptable");
        const char CSZ = 50;
        char content_type[CSZ];
        httpd_req_get_hdr_value_str(req, "Content-Type", content_type, CSZ);
        char content_msg[110]; //50+60
        sprintf(content_msg,"Error: header content-type must be 'application/json', not '%s'", content_type );

        httpd_resp_send(req, content_msg, HTTPD_RESP_USE_STRLEN);
        success = false;
        return ESP_FAIL;
    }
    if (success){
        ESP_LOGI(TAG_SERVER, "Parse of request success: '%s'", buf);
    } 
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void serve_title(httpd_req_t *req) {
    char chipid[UNIQUE_ID_LENGTH];
    get_chip_id(chipid, UNIQUE_ID_LENGTH);

    // Allocate memory for the format string
    size_t n_template = title_html_end - title_html_start;
    char* line_template = (char*)malloc(n_template);
    snprintf(line_template, n_template, "%s", title_html_start);

    // Allocate memory for the output string
    size_t needed = snprintf(NULL, 0, line_template, chipid)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, line_template, chipid);
    free(line_template);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
}

void serve_seat_and_lock(httpd_req_t *req){
    extern uint8_t desired_seat_number;
    char* lock_status = "TODO";

    // Allocate memory for the format string
    size_t n_template = subtitle_html_end - subtitle_html_start;
    char* line_template = (char*)malloc(n_template);
    snprintf(line_template, n_template, "%s", subtitle_html_start);

    // Display the seat number 1 higher than stored in the back end. 
    // Back end
    int ui_seat_number = desired_seat_number+1;
    size_t needed = snprintf(NULL, 0, line_template, ui_seat_number, lock_status)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, line_template, ui_seat_number, lock_status);
    free(line_template);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
    //ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
}

void serve_wificonfig(httpd_req_t *req){
    // Allocate memory for the format string
    size_t n_template = wifiSettings_html_end - wifiSettings_html_start;
    char* line_template = (char*)malloc(n_template);
    snprintf(line_template, n_template, "%s", wifiSettings_html_start);

    size_t needed = snprintf(NULL, 0, line_template, desired_ap_ssid, desired_ap_pass, desired_friendly_name, desired_mqtt_broker_ip)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, line_template, desired_ap_ssid, desired_ap_pass, desired_friendly_name, desired_mqtt_broker_ip);
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
}

void serve_settingsconfig(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)settings_html_start, settings_html_end - settings_html_start));
}

void serve_test(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)test_html_start, test_html_end - test_html_start));
}

void serve_html_beg(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)html_beg_start, html_beg_end - html_beg_start));
}

void serve_html_end(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)html_conc_start, html_conc_end - html_conc_start));
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, NULL, 0));
}

void serve_menu_bar(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, (const char *)menu_bar_start, menu_bar_end - menu_bar_start));
}

static esp_err_t wifi_get_handler(httpd_req_t *req)
{
    serve_html_beg(req);
    serve_title(req);
    serve_seat_and_lock(req);
    serve_menu_bar(req);
    serve_wificonfig(req);
    serve_html_end(req);
    return ESP_OK;
}


static esp_err_t settings_get_handler(httpd_req_t *req)
{
    serve_html_beg(req);
    serve_title(req);
    serve_seat_and_lock(req);
    serve_menu_bar(req);
    serve_settingsconfig(req);
    serve_html_end(req);
    return ESP_OK;
}

static esp_err_t test_cv_get_handler(httpd_req_t *req)
{   
    serve_html_beg(req);
    serve_title(req);
    serve_menu_bar(req);
    serve_test(req);
    serve_html_end(req);
    return ESP_OK;
}

/* jquery GET handler */
static esp_err_t cv_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG_SERVER, "cv_server requested");

	httpd_resp_set_type(req, "application/javascript");

	httpd_resp_send(req, (const char *)cv_js_start, (cv_js_end - cv_js_start)-1);

	return ESP_OK;
}

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = wifi_get_handler
};

static const httpd_uri_t wifi_uri = {
    .uri       = "/wifi_get",
    .method    = HTTP_GET,
    .handler   = wifi_get_handler
};

static const httpd_uri_t settings_uri = {
    .uri       = "/settings",
    .method    = HTTP_GET,
    .handler   = settings_get_handler
};

static const httpd_uri_t config_settings_uri = {
    .uri       = "/settings",
    .method    = HTTP_POST,
    .handler   = config_settings_post_handler
};

// // When a user manually submits wifi config. Issues a confirmation page
// static const httpd_uri_t config_wifi_uri = {
//     .uri       = "/wifi_config",
//     .method    = HTTP_POST,
//     .handler   = config_wifi_post_handler
// };

// Used to submit wifi credentials without promting or redirecting
// static const httpd_uri_t config_wifi_instant_uri = {
//     .uri   = "/wifi_instant",
//     .method    = HTTP_POST,
//     .handler   = config_wifi_post_handler
// };


static const httpd_uri_t test_uri = {
    .uri       = "/test",
    .method    = HTTP_GET,
    .handler   = test_cv_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
};

// static const httpd_uri_t config_test_uri = {
//     .uri       = "/test",
//     .method    = HTTP_POST,
//     .handler   = config_test_post_handler
// };


static const httpd_uri_t cv_js_uri = {
	.uri = "/cv_js.js",
	.method = HTTP_GET,
	.handler = cv_js_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};

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

        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &wifi_uri));
        //ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_wifi_uri));
        //ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_wifi_instant_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_settings_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &settings_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_index_html));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_favicon_ico));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_jquery_3_4_1_min_js));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_update));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &OTA_status));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &cv_js_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &test_uri));
        //ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_test_uri));


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