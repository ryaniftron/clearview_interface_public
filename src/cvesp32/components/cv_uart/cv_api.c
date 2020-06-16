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

// extern void reset_lock_cmd(char* buf){
//     char tbuf[MAX_CMD_LEN];
//     form_command("RL", tbuf); 
//     // Send it twice
//     sprintf(buf, "%s%s", tbuf, tbuf);

// }

// extern void set_videoformat_cmd(char* video_format, char* buf){
//     form_command_biparam("BC",video_format, buf);
// }

