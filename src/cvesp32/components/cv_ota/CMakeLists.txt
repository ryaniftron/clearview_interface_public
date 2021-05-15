idf_component_register(SRCS "cv_ota.c"
                       REQUIRES esp_http_server protocol_examples_common app_update
                       INCLUDE_DIRS "include"
                       EMBED_TXTFILES WebsiteFiles/favicon.ico WebsiteFiles/ota.html WebsiteFiles/jquery-3.4.1.min.js
                       )

