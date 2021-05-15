#pragma once

#include <esp_http_server.h>

void otaRebootTask(void * parameter);



extern esp_err_t OTA_index_html_handler(httpd_req_t *req);
extern esp_err_t OTA_favicon_ico_handler(httpd_req_t *req);
extern esp_err_t jquery_3_4_1_min_js_handler(httpd_req_t *req);
extern esp_err_t OTA_update_status_handler(httpd_req_t *req);
esp_err_t OTA_update_post_handler(httpd_req_t *req);
//extern esp_err_t OTA_index_html_handler(httpd_req_t *req);
