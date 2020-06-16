#pragma once
extern void form_command(char* payload, char* full_command);

extern bool set_antenna(char* antenna);
extern bool set_channel(char* channel);
extern bool set_band(char* band);
extern bool set_osdpos(char* osdpos);