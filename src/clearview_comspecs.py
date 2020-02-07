
# These are the proprietary communication specs that work with FTDI 57k
clearview_specs = {
    'message_start_char': '\n',
    'message_end_char': '\r',
    'message_csum': '!',
    'mess_src': 9,  # 9 for laptop
    'baud': 57600,
    'bc_id': 0,
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
    "frequency" : {
        "report_name":"RPFR",
        "pattern": "TODO", #TODO
        "reply_nt": "TODO" #TODO
    }
}