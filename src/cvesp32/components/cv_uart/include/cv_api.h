#pragma once
#include <esp_system.h>

enum  cv_api_code{
    CV_OK,
    CV_ERROR_NO_COMMS,
    CV_ERROR_INVALID_ENDPOINT,
    CV_ERROR_WRITE,
    CV_ERROR_READ,
    CV_ERROR_VALUE,
    CV_ERROR_NVS_READ,
    CV_ERROR_NVS_WRITE
};


struct cv_api_write {
    bool success; // Whether parameter was written
    enum cv_api_code api_code;
};

struct cv_api_read {
    bool success; // Whether parameter was read
    enum cv_api_code api_code; 
    char* val; // The value of the parameter, or error message if not succesful
};

extern int form_command(char* payload, char* output_command, int bufsz);

//https://stackoverflow.com/q/10162152/14180509

// CV Getters
extern struct cv_api_read get_channel();



// CV Setters
extern struct cv_api_write set_address(char* address);
extern struct cv_api_write set_antenna(char* antenna);
extern struct cv_api_write set_channel(char* channel);
extern struct cv_api_write set_band(char* band);
extern struct cv_api_write set_id(char* id_str);
extern struct cv_api_write set_usermsg(char* usermsg_str);
extern struct cv_api_write set_mode(char* mode_str);
extern struct cv_api_write set_osdvis(char* osdvis_str);
extern struct cv_api_write set_osdpos(char* osdpos);
extern struct cv_api_write reset_lock();
extern struct cv_api_write set_videoformat(char* videoformat);
extern struct cv_api_write set_custom(char* cmd);