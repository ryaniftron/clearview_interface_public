idf_component_register(SRCS "cv_ota.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server protocol_examples_common app_update cv_server
                       EMBED_TXTFILES WebsiteFiles/favicon.ico WebsiteFiles/ota.html WebsiteFiles/jquery-3.4.1.min.js
                       )

