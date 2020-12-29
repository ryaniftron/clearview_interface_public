idf_component_register(SRCS "cv_server.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server cv_mqtt cv_ota cv_wifi json
                       EMBED_TXTFILES
                           WebsiteFiles/settings.html
                           WebsiteFiles/httpDocBegin.html
                           WebsiteFiles/httpDocConclude.html
                           WebsiteFiles/subtitle.html
                           WebsiteFiles/title.html
                           WebsiteFiles/wifiSettings.html
                           WebsiteFiles/test.html
                           WebsiteFiles/menuBar.html
                           WebsiteFiles/menuBarTest.html
                           WebsiteFiles/cv_js.js
                           WebsiteFiles/cvcm_style.css
                       )

