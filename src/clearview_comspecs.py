
# These are the proprietary communication specs that work with FTDI 57k
clearview_specs = {
    'message_start_char' : '\n',
    'message_end_char' : '\r',
    'message_csum' : '!',
    'mess_src'  : 9,
    'baud' : 57600,
}