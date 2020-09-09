#pragma once
#include "esp_http_server.h"
extern httpd_handle_t start_cv_webserver(void);
extern void serve_title(httpd_req_t *req); 
extern void serve_seat_and_lock(httpd_req_t *req);
extern void serve_wificonfig(httpd_req_t *req);
extern void serve_settingsconfig(httpd_req_t *req);
extern void serve_test(httpd_req_t *req);
extern void serve_html_beg(httpd_req_t *req);
extern void serve_html_end(httpd_req_t *req);
extern void serve_menu_bar(httpd_req_t *req);
