#include "cv_api.h"
#include "stdio.h"
#include "cv_uart.h"
#include "string.h"

#include <esp_log.h>
#define START_CHAR 0x02
#define END_CHAR 0x03
#define CSUM "%"
#define RX_TARGET 0
#define MESS_SRC 9
#define MAX_CMD_LEN 64 //64 characters total



// Generates CV command from payload and puts into buffer output_command
extern int form_command(char* payload, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%c", START_CHAR, RX_TARGET, MESS_SRC,  payload, CSUM, END_CHAR);
    if (bufsz != 0) ESP_LOGI("CV_API", "CMD: '%s'", output_command);
    return n;
}

//Generates CV command from two appended payloads and puts into buffer output_command
extern int form_command_biparam(char* payload1, char* payload2, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%s%c", START_CHAR, RX_TARGET, MESS_SRC, payload1, payload2, CSUM, END_CHAR);
    if (bufsz != 0) ESP_LOGI("CV_API", "CMD: '%s'", output_command);
    return n;
}

extern int set_address_cmd(char* address, char* buf, int bufsz){
    return form_command_biparam("ADS",address, buf, bufsz);
}




extern struct cv_api_write set_address(char* address){
    struct cv_api_write ret;
    char* end;
    int addr_i = (int) strtol(address, &end, 10);
    if (addr_i < 0 || addr_i > 7) {
        ret.api_code=CV_ERROR_VALUE;
        ret.success=false;
        return ret;
    }
    size_t needed = set_address_cmd(address, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_address_cmd(address, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}


extern int get_channel_cmd(char* buf, int bufsz){
    return form_command("BC", buf, bufsz);
}

extern struct cv_api_read get_channel(){
    struct cv_api_read ret;

    //TODO is this causing a memory leak 
    char* cmd = (char*)malloc(MAX_CMD_LEN);
    uint8_t* rxbuf = (uint8_t*) malloc(RX_BUF_SIZE+1);
    get_channel_cmd(cmd, MAX_CMD_LEN);
    int report_len = cvuart_send_report(cmd, rxbuf);
    if (report_len == -1 || report_len == 0){
        ret.api_code = CV_ERROR_NO_COMMS;
        ret.success = false;
    } else {
        ret.api_code = CV_OK;
        ret.success = true;
        ret.val = (char*)rxbuf;
    }
    free(cmd);
    return ret;
}

extern int set_antenna_cmd(char* antenna_mode, char* buf, int bufsz){
    return form_command_biparam("AN",antenna_mode, buf, bufsz);
}

extern struct cv_api_write set_antenna(char* antenna){
    struct cv_api_write ret;
  
    if (!(antenna[0] == '0' || antenna[0] == '1' || antenna[0] == '2' || antenna[0] == '3')) {
        ret.api_code=CV_ERROR_VALUE;
        ret.success=false;
        return ret;
    }

    size_t needed = set_antenna_cmd(antenna, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_antenna_cmd(antenna, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}


extern int set_channel_cmd(char* channel, char* buf, int bufsz){
    return form_command_biparam("BC",channel, buf, bufsz);
}

extern struct cv_api_write set_channel(char* channel){
    struct cv_api_write ret;
    size_t needed = set_channel_cmd(channel, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_channel_cmd(channel, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int set_band_cmd(char* band, char* buf, int bufsz){
    return form_command_biparam("BG",band, buf, bufsz);
}

extern struct cv_api_write set_band(char* band){
    struct cv_api_write ret;
    size_t needed = set_band_cmd(band, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_band_cmd(band, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int set_id_cmd(char* id_str, char* buf, int bufsz){
    return form_command_biparam("ID", id_str, buf, bufsz);
}

extern struct cv_api_write set_id(char* id_str){
    struct cv_api_write ret;
    size_t needed = set_id_cmd(id_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_id_cmd(id_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int set_usermsg_cmd(char* usermsg_str, char* buf, int bufsz){
    return form_command_biparam("UM", usermsg_str, buf, bufsz);
}

extern struct cv_api_write set_usermsg(char* usermsg_str){
    struct cv_api_write ret;
    size_t needed = set_usermsg_cmd(usermsg_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_usermsg_cmd(usermsg_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int set_mode_cmd(char* mode_str, char* buf, int bufsz){
    return form_command_biparam("MD", mode_str, buf, bufsz);
}

// Set display mode
// * "L" - live video
// * "M" - menu
// * "S" - spectrum analyzer
extern struct cv_api_write set_mode(char* mode_str){
    struct cv_api_write ret;
    size_t needed = set_mode_cmd(mode_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_mode_cmd(mode_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}


extern int set_osdvis_cmd(char* osdvis_str, char* buf, int bufsz){
    return form_command_biparam("OD", osdvis_str, buf, bufsz);
}

// Set OSD Visibility
// * "E" - Enabled
// * "D" - Disabled
extern struct cv_api_write set_osdvis(char* osdvis_str){
    struct cv_api_write ret;
    size_t needed = set_osdvis_cmd(osdvis_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdvis_cmd(osdvis_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}


extern int set_osdpos_cmd(char* osd_pos, char* buf, int bufsz){
    return form_command_biparam("ODP", osd_pos, buf, bufsz);
}

extern struct cv_api_write set_osdpos(char* osdpos){
    struct cv_api_write ret;
    size_t needed = set_osdpos_cmd(osdpos, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdpos_cmd(osdpos, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int reset_lock_cmd(char* buf, int bufsz){
    char tbuf[MAX_CMD_LEN];
    form_command("RL", tbuf, bufsz); 
    // Send it twice
    return sprintf(buf, "%s%s", tbuf, tbuf);
}

extern struct cv_api_write reset_lock() {
    struct cv_api_write ret;
    size_t needed = reset_lock_cmd( NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    reset_lock_cmd(cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

extern int set_videoformat_cmd(char* video_format, char* buf, int bufsz){
    return form_command_biparam("VF",video_format, buf, bufsz);
}

// Set Video Format (camera type)
// * "N" - NTSC
// * "A" - Auto
// * "P" - PAL
extern struct cv_api_write set_videoformat(char* videoformat){
    struct cv_api_write ret;
    size_t needed = set_videoformat_cmd(videoformat, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_videoformat_cmd(videoformat, cmd, needed);
    printf("%s\n", cmd);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret.success = write_succ;
    if (write_succ)
        ret.api_code = CV_OK;
    else 
        ret.api_code = CV_ERROR_NO_COMMS;
    return ret;
}

