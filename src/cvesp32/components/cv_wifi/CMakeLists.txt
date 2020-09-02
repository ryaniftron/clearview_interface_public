idf_component_register(SRCS "cv_wifi.c" 
                       INCLUDE_DIRS "include"
                       REQUIRES cv_ledc cv_utils cv_server
                       )
set_source_files_properties(cv_wifi.c
    PROPERTIES COMPILE_FLAGS
    -Wno-enum-compare
)