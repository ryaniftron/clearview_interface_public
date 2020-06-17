idf_component_register(SRCS "cv_server.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server cv_mqtt cv_ota
                       EMBED_TXTFILES WebsiteFiles/index2.html 
                       )

