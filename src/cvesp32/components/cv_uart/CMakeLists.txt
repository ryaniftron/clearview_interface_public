idf_component_register(SRCS "cv_api.c" "cv_uart.c"
                       REQUIRES app_update cv_utils
                       INCLUDE_DIRS "include"
                       )