#pragma once

#include <esp_http_server.h>

void otaRebootTask(void * parameter);

extern httpd_uri_t OTA_index_html;
extern httpd_uri_t OTA_favicon_ico;
extern httpd_uri_t OTA_jquery_3_4_1_min_js;
extern httpd_uri_t OTA_update;
extern httpd_uri_t OTA_status;
