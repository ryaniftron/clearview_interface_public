#set(COMPONENT_SRCDIRS ".")
#set(COMPONENT_ADD_INCLUDEDIRS "include")
#set(COMPONENT_PRIV_REQUIRES nvs_flash mqtt_client)
#register_component()

idf_component_register(SRCS "cv_mqtt.c"
                       INCLUDE_DIRS "." "include"
                       REQUIRES nvs_flash 
                       )

