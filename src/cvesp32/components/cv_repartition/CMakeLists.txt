cmake_minimum_required(VERSION 3.5)
idf_component_register(SRCS "cv_repartition.c"
                       INCLUDE_DIRS "include"
                       REQUIRES app_update
                       )
