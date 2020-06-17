#include "stdio.h"
#include "cv_uart.h"
#include "string.h"
#define START_CHAR 0x02
#define END_CHAR 0x03
#define CSUM "%"
#define RX_TARGET 0
#define MESS_SRC 9
#define MAX_CMD_LEN 64 //64 characters total

// Generates CV command from payload and puts into buffer output_command
extern int form_command(char* payload, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%c", START_CHAR, RX_TARGET, MESS_SRC,  payload, CSUM, END_CHAR);
    
    return n;
}

//Generates CV command from two appended payloads and puts into buffer output_command
extern int form_command_biparam(char* payload1, char* payload2, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%s%c", START_CHAR, RX_TARGET, MESS_SRC, payload1, payload2, CSUM, END_CHAR);
    return n;
}

extern int set_address_cmd(char* address, char* buf, int bufsz){
    return form_command_biparam("ADS",address, buf, bufsz);
}

extern bool set_address(char* address){
    size_t needed = set_address_cmd(address, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_address_cmd(address, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_antenna_cmd(char* antenna_mode, char* buf, int bufsz){
    return form_command_biparam("AN",antenna_mode, buf, bufsz);
}

extern bool set_antenna(char* antenna){
    size_t needed = set_antenna_cmd(antenna, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_antenna_cmd(antenna, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}


extern int set_channel_cmd(char* channel, char* buf, int bufsz){
    return form_command_biparam("BC",channel, buf, bufsz);
}

extern bool set_channel(char* channel){
    size_t needed = set_channel_cmd(channel, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_channel_cmd(channel, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_band_cmd(char* band, char* buf, int bufsz){
    return form_command_biparam("BG",band, buf, bufsz);
}

extern bool set_band(char* band){
    size_t needed = set_band_cmd(band, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_band_cmd(band, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_id_cmd(char* id_str, char* buf, int bufsz){
    return form_command_biparam("ID", id_str, buf, bufsz);
}

extern bool set_id(char* id_str){
    size_t needed = set_id_cmd(id_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_id_cmd(id_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_usermsg_cmd(char* usermsg_str, char* buf, int bufsz){
    return form_command_biparam("UM", usermsg_str, buf, bufsz);
}

extern bool set_usermsg(char* usermsg_str){
    size_t needed = set_usermsg_cmd(usermsg_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_usermsg_cmd(usermsg_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_mode_cmd(char* mode_str, char* buf, int bufsz){
    return form_command_biparam("MD", mode_str, buf, bufsz);
}

// Set display mode
// * "L" - live video
// * "M" - menu
// * "S" - spectrum analyzer
extern bool set_mode(char* mode_str){
    size_t needed = set_mode_cmd(mode_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_mode_cmd(mode_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}


extern int set_osdvis_cmd(char* osdvis_str, char* buf, int bufsz){
    return form_command_biparam("OD", osdvis_str, buf, bufsz);
}

// Set OSD Visibility
// * "E" - Enabled
// * "D" - Disabled
extern bool set_osdvis(char* osdvis_str){
    size_t needed = set_osdvis_cmd(osdvis_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdvis_cmd(osdvis_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}


extern int set_osdpos_cmd(char* osd_pos, char* buf, int bufsz){
    return form_command_biparam("ODP", osd_pos, buf, bufsz);
}

extern bool set_osdpos(char* osdpos){
    size_t needed = set_osdpos_cmd(osdpos, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdpos_cmd(osdpos, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int reset_lock_cmd(char* buf, int bufsz){
    char tbuf[MAX_CMD_LEN];
    form_command("RL", tbuf, bufsz); 
    // Send it twice
    return sprintf(buf, "%s%s", tbuf, tbuf);
}

extern bool reset_lock() {
    size_t needed = reset_lock_cmd( NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    reset_lock_cmd(cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

extern int set_videoformat_cmd(char* video_format, char* buf, int bufsz){
    return form_command_biparam("VF",video_format, buf, bufsz);
}

// Set Video Format (camera type)
// * "N" - NTSC
// * "A" - Auto
// * "P" - PAL
extern bool set_videoformat(char* videoformat){
    size_t needed = set_videoformat_cmd(videoformat, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_videoformat_cmd(videoformat, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    return write_succ;
}

