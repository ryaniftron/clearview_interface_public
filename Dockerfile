FROM espressif/idf:release-v4.2
RUN apt-get update
RUN echo '$IDF_PATH/install.sh' >> ~/.bashrc
RUN echo '. /opt/esp/idf/export.sh' >> ~/.bashrc