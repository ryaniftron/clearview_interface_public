#pragma once
extern void form_command(char* payload, char* full_command);

extern bool set_address(char* address);
extern bool set_antenna(char* antenna);
extern bool set_channel(char* channel);
extern bool set_band(char* band);
extern bool set_id(char* id_str);
extern bool set_usermsg(char* usermsg_str);
extern bool set_mode(char* mode_str);
extern bool set_osdvis(char* osdvis_str);
extern bool set_osdpos(char* osdpos);
extern bool reset_lock();
extern bool set_videoformat(char* videoformat);