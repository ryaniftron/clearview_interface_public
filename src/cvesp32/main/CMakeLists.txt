idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(
    SRCS "cv_utils.c" "cv_server.c" "cv_ledc.c" "cv_ota.c" "cv_uart.c" "main_cv_mqtt.c" "cv_server.c" "cv_utils.c" "main.c"
    INCLUDE_DIRS ""
    EMBED_TXTFILES ${project_dir}/main/WebsiteFiles/favicon.ico ${project_dir}/main/WebsiteFiles/index.html  ${project_dir}/main/WebsiteFiles/jquery-3.4.1.min.js
)
