# These are the proprietary communication specs that work with FTDI 57k
clearview_specs = {
    'message_start_char': '\x02',
    'message_end_char': '\x03',
    # 'message_start_char': '\r',
    # 'message_end_char': '\n',
    'message_csum': '%',
    'mess_src': 9,  # 9 for laptop
    'baud': 57600,
    'bc_id': 0,
}

cv_device_limits = {
    'min_seat_number': 1,   # set receiver address target
    'max_seat_number': 8, #docs says this is actual 0-9,a-f but implementation seems to differ
    'min_destination': 0,
    'max_destination': 9,
    'min_source_addr': 1,
    'max_source_addr': 9,
    'min_antenna_mode': 0,
    'max_antenna_mode': 3,
    'min_channel': 0,   # band channel
    'max_channel': 7,
    'min_frequency': 5200,
    'max_frequency': 5999,
    'max_id_length': 12,    # Used for pilot handle. More than this limit breaks the parser
    'max_um_length': 45,     # Rough limit. OSD will truncate
    'min_osd_position': 0,
    'max_osd_position': 7,
    'video_format_list': ['N', 'P', 'A'],
    'video_formats': "[NPA]",  # camera type = NTSC, AUTO, PAL, also RPLF arg1
    'lock_unlock': "[LMU]",   # RPLF, arg2
    'force_auto': "[FA]",    #RPLF, arg3
    'video_static': "[VS]",  #RPLF, arg4
    'antenna_modes': "[0-3]",
    'bands': "[0-9,a-f]",
    'video_modes': "[LSM]",     # live, spectrum, menu
    'osd_visibilities': "[ED]",  # Enable, Disable
    'osd_positions': "[0-7]",
    'osd_text_chars': "[^%s%s%s]"%(
        clearview_specs["message_start_char"],
        clearview_specs["message_end_char"],
       clearview_specs["message_csum"]
    ),
    'cursor_commands': "[+-EPMX]",
    'wifi_mode_ap': "ap",
    'wifi_mode_sta': "sta"
}


# key = letter or number to send to clearview
# value = Displayed Band Name
band_map_cv_to_display = {
    '0': 'A',
    '1': 'B',
    '2': 'R',
    '3': 'E',
    '4': 'F',
    '5': 'L',
    '6': 'H',
    '7': 'U',
    '8': 'I',    # (uppercase letter i)
    '9': 'J',
    'a': 'S',
    'b': 'M',
    'c': 'Q',
    'd': 'X',
    'e': 'Y',
    'f': 'Z',
}

# Map the reverse direction for ease of use
band_map_display_to_cv = {v: k for k, v in band_map_cv_to_display.items()}

report_standards = {
    "frequency": {
        "report_name": "RPFR",
        "pattern": "TODO",  # TODO
        "reply_nt": "TODO"  # TODO
    }
}


band_to_frequency_table = {
    'R1': 5658,
    'R2': 5695,
    'R3': 5732,
    'R4': 5769,
    'R5': 5806,
    'R6': 5843,
    'R7': 5880,
    'R8': 5917,
    'F1': 5740,
    'F2': 5760,
    'F3': 5780,
    'F4': 5800,
    'F5': 5820,
    'F6': 5840,
    'F7': 5860,
    'F8': 5880,
    'E1': 5705,
    'E2': 5685,
    'E3': 5665,
    'E4': 5645,
    'E5': 5885,
    'E6': 5905,
    'E7': 5925,
    'E8': 5945,
    'B1': 5733,
    'B2': 5752,
    'B3': 5771,
    'B4': 5790,
    'B5': 5809,
    'B6': 5828,
    'B7': 5847,
    'B8': 5866,
    'A1': 5865,
    'A2': 5845,
    'A3': 5825,
    'A4': 5805,
    'A5': 5785,
    'A6': 5765,
    'A7': 5745,
    'A8': 5725,
    'L1': 5362,
    'L2': 5399,
    'L3': 5436,
    'L4': 5473,
    'L5': 5510,
    'L6': 5547,
    'L7': 5584,
    'L8': 5621,
    'U0': 5300,
    'U1': 5325,
    'U2': 5348,
    'U3': 5366,
    'U4': 5384,
    'U5': 5402,
    'U6': 5420,
    'U7': 5438,
    'U8': 5456,
    'U9': 5985,
    #'D1': 5660,
    #'D2': 5695,
    #'D3': 5735,
    #'D4': 5770,
    #'D5': 5805,
    #'D6': 5878,
    #'D7': 5914,
    #'D8': 5839,
}

frequency_to_band_table = {}
for b in band_to_frequency_table:
    f = band_to_frequency_table[b]
    frequency_to_band_table[f]=b


def frequency_to_bandchannel(frequency):
    try:
        return frequency_to_band_table[frequency]
    except KeyError:
        print("Frequency not allowed in clearview")
        return None

def band_to_frequency(band):
    try:
        return band_to_frequency_table[band]
    except KeyError:
        return None

def band_name_to_cv_index(band):
    try:
        return band_map_display_to_cv[band]
    except KeyError:
        return None

# Return {band:X, Channel:X} given a frequency
def frequency_to_bandchannel_dict(frequency):
    bc = frequency_to_bandchannel(frequency)
    if bc is not None:
        band = band_map_display_to_cv[bc[0]]
        channel = str(int(bc[1]) - 1)
        bc_dict = {
            "band":band,
            "channel":channel
        }
        return bc_dict
    else:
        return None