idf_component_register(SRCS "tbs_mqtt.c"
                       INCLUDE_DIRS "include"
                       REQUIRES nvs_flash protocol_examples_common mqtt cv_uart cv_ledc cv_utils
                       )

