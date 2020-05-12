
#include <esp_event.h>
#include <esp_log.h>

#include "lwip/api.h"
#include "esp_http_server.h"
#include "cv_utils.c"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b)) //where does this come from? see http_server_simple example
#endif //MIN

const char* TAG_SERVER = "CV_SERVER";

#define HTML_ROOT_TITLE \
"<h2><center>ClearView Wireless %s  </center></h2>" //supply chip_id

#define HTML_ROOT_SUBTITLE \
"<table style=\"table-layout: fixed; width: 100\%%;\">\
  <tr>\
    <td style=\"text-align:left; width:50\%%\">Node: %d</td>\
    <td style=\"text-align:left;  width:50\%%\"> Lock: %s</td>\
  </tr>\
</table>\
<hr>"

// '/' , the root text configuration
#define HTML_WIFICONFIG \
    "<form action=\"/config_wifi\" method> \
        <label for=\"ssid\">Network SSID:</label><br> \
        <input type=\"text\" id=\"ssid\" name=\"ssid\"><br> \
        <label for=\"password\">Network Password:</label><br> \
        <input type=\"text\" id=\"password\" name=\"password\"><br> \
        <label for=\"device_name\">Device Name:</label><br> \
        <input type=\"text\" id=\"device_name\" name=\"device_name\"><br> \
        <label for=\"broker_ip\">Broker IP Address:</label><br> \
        <input type=\"text\" id=\"broker_ip\" name=\"broker_ip\"><br> \
        <input type=\"submit\" value=\"Save and Join Network\">\
    "

// '/settings' body
#define HTML_SETTINGS \
    "<form action=\"/config_settings\" > \
    <label for=\"nodeN\">Node Number:</label>\
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
    <input type=\"submit\" value=\"Set Node Number\">\
    "


//TODO make these a struct
bool ssid_ready = false;
bool password_ready = false;
bool device_name_ready = false;
bool broker_ip_ready = false;

static esp_err_t config_settings_get_handler(httpd_req_t *req)
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
            if (httpd_query_key_value(buf, "node_number", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_SERVER, "Found URL query parameter => node_number=%s", param);
                set_credential("node_number", param);
            }
        }
        free(buf);
        httpd_resp_send_chunk(req, "Settings Accepted", HTTPD_RESP_USE_STRLEN);    
        httpd_resp_send_chunk(req, NULL, 0);
        }
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
                ssid_ready = true;
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
            ESP_LOGI(TAG_SERVER, "All WiFi Creds received");
            char* redirect_str = "<meta http-equiv=\"Refresh\" content=\"3\"; url=\"https://192.168.0.1/\" />";
            httpd_resp_send_chunk(req, redirect_str, HTTPD_RESP_USE_STRLEN);    
            httpd_resp_send_chunk(req, NULL, 0);
            extern bool switch_to_sta;
            vTaskDelay(4000 / portTICK_PERIOD_MS);
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
    extern int node_number;
    char* lock_status = "TODO";

    size_t needed = snprintf(NULL, 0, HTML_ROOT_SUBTITLE, node_number, lock_status)+1;
    char* line = (char*)malloc(needed);
    snprintf(line, needed, HTML_ROOT_SUBTITLE, node_number, lock_status);
    //ESP_LOGI(TAG_SERVER, "Node and Lock Request %d ,%s", node_number, "TODO");
    // size_t needed = snprintf(NULL, 0, HTML_ROOT_SUBTITLE, node_number, lock_status);
    
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
    free(line);
    //ESP_ERROR_CHECK(httpd_resp_send_chunk(req, line, HTTPD_RESP_USE_STRLEN));
}

void serve_wificonfig(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, HTML_WIFICONFIG, HTTPD_RESP_USE_STRLEN));
}

void serve_settingsconfig(httpd_req_t *req){
    ESP_ERROR_CHECK(httpd_resp_send_chunk(req, HTML_SETTINGS, HTTPD_RESP_USE_STRLEN));
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

static const httpd_uri_t config_settings_uri = {
    .uri       = "/config_settings",
    .method    = HTTP_GET,
    .handler   = config_settings_get_handler
};

static const httpd_uri_t config_wifi_uri = {
    .uri       = "/config_wifi",
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

// /* An HTTP POST handler */
// static esp_err_t echo_cv_post_handler(httpd_req_t *req)
// {
//     char buf[100];
//     int ret, remaining = req->content_len;

//     while (remaining > 0) {
//         /* Read the data for the request */
//         if ((ret = httpd_req_recv(req, buf,
//                         MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 /* Retry receiving if timeout occurred */
//                 continue;
//             }
//             return ESP_FAIL;
//         }

//         /* Send back the same data */
//         httpd_resp_send_chunk(req, buf, ret);
//         remaining -= ret;

//         /* Log data received */
//         ESP_LOGI(TAG_SERVER, "=========== RECEIVED DATA ==========");
//         ESP_LOGI(TAG_SERVER, "%.*s", ret, buf);
//         ESP_LOGI(TAG_SERVER, "====================================");
//     }

//     // End response
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
// esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
// {
//     if (strcmp("/hello", req->uri) == 0) {
//         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
//         /* Return ESP_OK to keep underlying socket open */
//         return ESP_OK;
//     } else if (strcmp("/echo", req->uri) == 0) {
//         httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
//         /* Return ESP_FAIL to close underlying socket */
//         return ESP_FAIL;
//     }
//     /* For any other URI send 404 and close socket */
//     httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
//     return ESP_FAIL;
// }


static httpd_handle_t start_cv_webserver(void){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG_SERVER, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG_SERVER, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello_uri);
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &config_wifi_uri);
        httpd_register_uri_handler(server, &config_settings_uri);
        httpd_register_uri_handler(server, &settings_uri);

        return server;
    }

    ESP_LOGI(TAG_SERVER, "Error starting server!");
    return NULL;
}


static void stop_cv_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
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