#include "cv_api.h"
#include "stdio.h"
#include "cv_uart.h"
#include "string.h"

#include "esp_ota_ops.h"
#include <esp_log.h>
#include "cv_utils.h"



#define TAG_API "CV_API"

#define START_CHAR 0x02
#define END_CHAR 0x03
#define CSUM "%"
// #define START_CHAR 0x41
// #define END_CHAR 0x42
// #define CSUM "!"

#define RX_TARGET 0
#define MESS_SRC 9
#define MAX_CMD_LEN 64 //64 characters total

#define CMD_BAND "BG"
#define REP_BAND "RPBG"
#define CMD_CHANNEL "BC"
#define REP_CHANNEL "RPBC"
#define CMD_ADDRESS "AD"
#define REP_ADDRESS "RPAD"
#define CMD_ANTENNA "AN"
#define REP_ANTENNA "RPAN"
#define CMD_ID "ID"
#define REP_ID "RPID"
#define CMD_RESET_LOCK "RL"
#define REP_LOCK_FMT "RPLF"
#define CMD_UM "UM"
#define REP_UM "RPUM"
#define CMD_MODE "MD"
#define REP_MODE "RPMD"
#define REP_CV_VER "RPMV"
#define CMD_VIDEO_FORMAT "VF"
#define REP_VIDEO_FORMAT "RPVF"
#define CMD_OSD_VIS "OD"
#define REP_OSD_VIS "RPOD"
#define CMD_OSD_POS "ODP"
#define REP_OSD_POS "RPODP"


// Generates CV command from payload and puts into buffer output_command
extern int form_command(char* payload, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%c", START_CHAR, RX_TARGET, MESS_SRC,  payload, CSUM, END_CHAR);
    if (bufsz != 0) ESP_LOGI(TAG_API, "CMD: '%s'", output_command);
    return n;
}

//Generates CV command from two appended payloads and puts into buffer output_command
extern int form_command_biparam(char* payload1, char* payload2, char* output_command, int bufsz) {
    int n = snprintf(output_command, bufsz, "%c%d%d%s%s%s%c", START_CHAR, RX_TARGET, MESS_SRC, payload1, payload2, CSUM, END_CHAR);
    if (bufsz != 0) ESP_LOGI(TAG_API, "CMD: '%s'", output_command);
    return n;
}

/* Take a command read from CV UART in full_cmd and return the whole payload
   This drops the source and destination
   Returns success if the command seems to be a valid format

    usage:
        char* full_cmd = "A09FR5740!B";
        char* payload = malloc(strlen(full_cmd));
        parse_command_payload(full_cmd, payload);
        //do stuff
        free(payload);
    */
extern bool parse_command_payload(char* full_cmd, char* payload){
    if (full_cmd == NULL || payload == NULL) {
        ESP_LOGE(TAG_API, "parse_command_payload; Empty cmd or payload");
        return false;
    }
    // ESP_LOGI(TAG_API,"full_cmd_infunc: '%s'\n", full_cmd);
    // TODO The fmt A%*c%*c.. doens't match only A
    char* fmt_base = "%c%s%%[^%s]%s%c";
    int n = snprintf(NULL, 0, fmt_base, START_CHAR,"%*c%*c", CSUM,CSUM, END_CHAR);
    char* fmt = malloc(++n);
    snprintf(fmt, n, fmt_base, START_CHAR,"%*c%*c", CSUM,CSUM, END_CHAR);
    // ESP_LOGI(TAG_API, "fmt:%s\n",fmt);
    sscanf(full_cmd, fmt, payload);
    // ESP_LOGI(TAG_API, "payload1 %s\n", payload);
    free(fmt);
    // ESP_LOGI(TAG_API, "n=%i\n",n);
    // ESP_LOGI(TAG_API, "L=%u\n", strlen(payload));

    return strlen(payload) != 0;
}


/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdbool.h>

#define START_CHAR 0x41
#define END_CHAR 0x42
#define CSUM "%"
#define RX_TARGET 0
#define MESS_SRC 9

void create_fmt_singlechar(char* accept, char* )

extern int parse_command_payload(char* full_cmd, char* payload){
    if (full_cmd == NULL) {
        //ESP_LOGE(TAG_API, "parse_command_payload; Empty cmd or payload");
        return false;
    }
    char* fmt_base = "%*[%c]%s%[^%s]%s%*[%c]";
    int n = snprintf(NULL, 0, fmt_base, START_CHAR,"%*c%*c", CSUM,CSUM, END_CHAR);
    char* fmt = malloc(++n);
    snprintf(fmt, n, fmt_base, START_CHAR,"%*c%*c", CSUM,CSUM, END_CHAR);
    printf("fmt:%s\n",fmt);
    sscanf(full_cmd, fmt, payload);
    printf("payload1 %s\n", payload);
    free(fmt);
    printf("n=%i\n",n);
    printf("L=%lu\n", strlen(payload));

    return strlen(payload) != 0;
}

int main () {
    char* full_cmd = "A09FR5740!B";\
    printf("full_cmd: '%s'\n", full_cmd);
    char* payload = malloc(strlen(full_cmd));
    bool r = parse_command_payload(full_cmd, payload);
    if(r)
     ESP_LOGI("TEST1", "%s", payload);
    else
     ESP_LOGI("TEST1", "fail");

    full_cmd = "X09FR5740!B";\
    r = parse_command_payload(full_cmd, payload);
    if(r)
     ESP_LOGI("TEST2", "%s", payload);
    else
     ESP_LOGI("TEST2!", "fail");


    //do stuff
    free(payload);;
}
*/

//Given a full_report and report_len, extract the data field from the report into ret
extern void process_report(struct cv_api_read* ret, char* full_report, int report_len, const char* report_name){
    if (report_len == -1 || report_len == 0){
        ret->api_code = CV_ERROR_NO_COMMS;
        ret->success = false;
        ESP_LOGW(TAG_API, "Can't process empty report");
        return;
    }
    ESP_LOGI(TAG_API, "Preprocess full_report %s\n", full_report);
    char* report_snippet = malloc(strlen(full_report)+1); //TODO ensure this is freed
    bool parse_tot_succ = parse_command_payload(full_report,report_snippet);
    ESP_LOGI(TAG_API, "Processed '%s' -> '%s'", full_report, report_snippet);

    if(!parse_tot_succ){
        ret->api_code = CV_ERROR_PARSE;
        ret->success = false;
    } else {
        ret->api_code = CV_OK;
        ret->success = true;
        ret->val = report_snippet + strlen(report_name);
    }
}

// Request a report of cmd_id and return the data in ret's struct
void run_read(struct cv_api_read* ret, char* cmd_id){
    char* cmd = (char*)malloc(MAX_CMD_LEN);
    uint8_t* rxbuf = (uint8_t*) malloc(RX_BUF_SIZE+1);
    form_command(cmd_id, cmd, MAX_CMD_LEN);
    int report_len = cvuart_send_report(cmd, rxbuf);

    process_report(ret, (char*)rxbuf, report_len, cmd_id+2); //skip 2 for 'RP'
    free(cmd);
}

extern int set_address_cmd(char* address, char* buf, int bufsz){
    return form_command_biparam("ADS",address, buf, bufsz);
}

extern void set_address(char* address, struct cv_api_write* ret){
    char* end;
    int addr_i = (int) strtol(address, &end, 10);
    if (addr_i < 0 || addr_i > 7) {
        ret->api_code=CV_ERROR_VALUE;
        ret->success=false;
        return;
    }
    size_t needed = set_address_cmd(address, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_address_cmd(address, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_antenna(struct cv_api_read* ret){
    char* cmd_id = REP_ANTENNA;
    run_read(ret, cmd_id);
}

extern int set_antenna_cmd(char* antenna_mode, char* buf, int bufsz){
    return form_command_biparam("AN",antenna_mode, buf, bufsz);
}

extern void set_antenna(char* antenna, struct cv_api_write* ret){
    if (!(antenna[0] == '0' || antenna[0] == '1' || antenna[0] == '2' || antenna[0] == '3')) {
        ret->api_code=CV_ERROR_VALUE;
        ret->success=false;
        return;
    }

    size_t needed = set_antenna_cmd(antenna, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_antenna_cmd(antenna, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}


extern void get_band(struct cv_api_read* ret){
    char* cmd_id = REP_BAND;
    run_read(ret, cmd_id);
}

extern int set_band_cmd(char* band, char* buf, int bufsz){
    return form_command_biparam("BG",band, buf, bufsz);
}

extern void set_band(char* band, struct cv_api_write* ret){
    size_t needed = set_band_cmd(band, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_band_cmd(band, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}


extern void get_channel(struct cv_api_read* ret){
    char* cmd_id = REP_CHANNEL;
    run_read(ret, cmd_id);
}

extern int set_channel_cmd(char* channel, char* buf, int bufsz){
    return form_command_biparam("BC",channel, buf, bufsz);
}

extern void set_channel(char* channel, struct cv_api_write* ret){
    size_t needed = set_channel_cmd(channel, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_channel_cmd(channel, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_id(struct cv_api_read* ret){
    char* cmd_id = REP_ID;
    run_read(ret, cmd_id);
}

extern int set_id_cmd(char* id_str, char* buf, int bufsz){
    return form_command_biparam("ID", id_str, buf, bufsz);
}

extern void set_id(char* id_str, struct cv_api_write* ret){
    size_t needed = set_id_cmd(id_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_id_cmd(id_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_usermsg(struct cv_api_read* ret){
    char* cmd_id = REP_UM;
    run_read(ret, cmd_id);
}

extern int set_usermsg_cmd(char* usermsg_str, char* buf, int bufsz){
    return form_command_biparam("UM", usermsg_str, buf, bufsz);
}

extern void set_usermsg(char* usermsg_str, struct cv_api_write* ret){
    size_t needed = set_usermsg_cmd(usermsg_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_usermsg_cmd(usermsg_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}
extern void get_mode(struct cv_api_read* ret){
    char* cmd_id = REP_MODE;
    run_read(ret, cmd_id);
}

extern int set_mode_cmd(char* mode_str, char* buf, int bufsz){
    return form_command_biparam("MD", mode_str, buf, bufsz);
}

// Set display mode
// * "L" - live video
// * "M" - menu
// * "S" - spectrum analyzer
extern void set_mode(char* mode_str, struct cv_api_write* ret){
    size_t needed = set_mode_cmd(mode_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_mode_cmd(mode_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_osdvis(struct cv_api_read* ret){
    char* cmd_id = REP_OSD_VIS;
    run_read(ret, cmd_id);
}

extern int set_osdvis_cmd(char* osdvis_str, char* buf, int bufsz){
    return form_command_biparam("OD", osdvis_str, buf, bufsz);
}

// Set OSD Visibility
// * "E" - Enabled
// * "D" - Disabled
extern void set_osdvis(char* osdvis_str, struct cv_api_write* ret){
    size_t needed = set_osdvis_cmd(osdvis_str, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdvis_cmd(osdvis_str, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_osdpos(struct cv_api_read* ret){
    char* cmd_id = REP_OSD_POS;
    // run_read(ret, cmd_id);
    ret->api_code = CV_ERROR_READ;
    ret->success = false;
}

extern int set_osdpos_cmd(char* osd_pos, char* buf, int bufsz){
    return form_command_biparam("ODP", osd_pos, buf, bufsz);
}

extern void set_osdpos(char* osdpos, struct cv_api_write* ret){
    size_t needed = set_osdpos_cmd(osdpos, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_osdpos_cmd(osdpos, cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern int reset_lock_cmd(char* buf, int bufsz){
    char tbuf[MAX_CMD_LEN];
    form_command(CMD_RESET_LOCK, tbuf, bufsz);
    // Send it twice
    printf("%s%s", tbuf, tbuf);
    return sprintf(buf, "%s%s", tbuf, tbuf);
}

extern void reset_lock(struct cv_api_write* ret) {
    size_t needed = reset_lock_cmd( NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    reset_lock_cmd(cmd, needed);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_lock(struct cv_api_read* ret){
    char* cmd_id = REP_LOCK_FMT;
    run_read(ret, cmd_id);
}

extern void get_videoformat(struct cv_api_read* ret){
    char* cmd_id = REP_VIDEO_FORMAT;
    run_read(ret, cmd_id);
}

extern int set_videoformat_cmd(char* video_format, char* buf, int bufsz){
    return form_command_biparam("VF",video_format, buf, bufsz);
}

// Set Video Format (camera type)
// * "N" - NTSC
// * "A" - Auto
// * "P" - PAL
extern void set_videoformat(char* videoformat, struct cv_api_write* ret){
    size_t needed = set_videoformat_cmd(videoformat, NULL, 0) + 1;
    char* cmd = (char*)malloc(needed);
    set_videoformat_cmd(videoformat, cmd, needed);
    printf("%s\n", cmd);
    bool write_succ = cvuart_send_command(cmd);
    free(cmd);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern int set_custom_cmd(char* cmd, char* buf, int bufsz) {
    return form_command(cmd, buf, bufsz);
}


extern void set_custom_w(char* cmd_des, struct cv_api_write* ret){
    size_t needed = set_custom_cmd(cmd_des, NULL, 0) + 1;
    char* cmd_tot = (char*)malloc(needed);
    set_custom_cmd(cmd_des, cmd_tot, needed);
    bool write_succ = cvuart_send_command(cmd_tot);
    free(cmd_tot);
    ret->success = write_succ;
    if (write_succ)
        ret->api_code = CV_OK;
    else
        ret->api_code = CV_ERROR_NO_COMMS;
}

extern void get_custom_report(char* report, struct cv_api_read* ret){
    size_t needed = set_custom_cmd(report, NULL, 0) + 1;
    char* cmd_tot = (char*)malloc(needed);
    set_custom_cmd(report, cmd_tot, needed);
    uint8_t* rxbuf = (uint8_t*) malloc(RX_BUF_SIZE+1);
    int report_len = cvuart_send_report(cmd_tot, rxbuf);
    if (report_len == -1 || report_len == 0){
        ret->api_code = CV_ERROR_NO_COMMS;
        ret->success = false;
    } else {
        ret->api_code = CV_OK;
        ret->success = true;
        ret->val = (char*)rxbuf;
    }
    free(cmd_tot);
}

extern void get_cv_version(struct cv_api_read* ret){
    char* cmd_id = REP_CV_VER;
    run_read(ret, cmd_id);
}

extern void get_cvcm_version(struct cv_api_read* ret){
    const int VER_LEN = 32;
    ret->val = strndup(esp_ota_get_app_description()->version, VER_LEN); //TODO memory
    ret->api_code=CV_OK;
    ret->success=true;
}

extern void get_cvcm_version_all(struct cv_api_read* ret){
    const int VER_LEN = 100;
    char fullversion[VER_LEN];
    sprintf(fullversion, "%s - %s - %s",(char*) esp_ota_get_app_description()->version,__DATE__, __TIME__);
    ret->val = strndup(fullversion, VER_LEN);
    ret->api_code=CV_OK;
    ret->success=true;
}

extern void get_mac_addr(struct cv_api_read* ret){
    char buf[UNIQUE_ID_LENGTH];
    get_chip_id(buf, UNIQUE_ID_LENGTH);
    ret->val = strndup(buf, UNIQUE_ID_LENGTH);
}