from __future__ import print_function
try:
    import queue
except ImportError:
    import Queue as queue
import json
import atexit
import logging
from collections import namedtuple
import re

import string

from clearview.comspecs import clearview_specs, cv_device_limits
logging.basicConfig()

import sys

logging.basicConfig()
logger = logging.getLogger(__name__)


# These are all the command types that can get sent to a CV
command_identifier_dict = {
    "set_address": 'ADS',
    "set_antenna_mode": 'AN',
    "set_channel": 'BC',
    "set_band": 'BG',
    "set_video_mode": 'MD',
    "set_osd_visibility": 'OD',
    "set_osd_position": 'ODP',
    "set_id":  'ID',
    "reset_lock": 'RL',
    "set_video_format": 'VF',
    "get_address": 'RPAD',
    "get_channel": 'RPBC',
    "get_band": 'RPBG',
    "get_frequency": 'RPFR',
    "get_osd_string": 'RPID',
    "get_lock_format": 'RPLF',
    "get_mode": 'RPMD',
    "get_model_version": 'RPMV',
    "get_rssi": 'RPRS',
    "get_osd_state": 'RPOD',
    "get_video_format": 'RPVF',
}

# These are all the response types that return from a CV
response_identifier_dict = {
    "get_address": 'AD',
    "get_channel": 'BC',
    "get_band": 'BG',
    "get_frequency": 'FR',
    "get_osd_string": 'ID',
    "get_lock_format": 'LF',
    "get_mode": 'MD',
    "get_model_version": 'MV',
    "get_rssi": 'RS',
    "get_osd_state": 'OD',
    "get_video_format": 'VF',
}

command_identifier_set = command_identifier_dict.values()
response_identifier_set = response_identifier_dict.values()


general_formatter = {
    "start_char" : clearview_specs["message_start_char"],
    "end_char": clearview_specs["message_end_char"],
    "csum": clearview_specs["message_csum"]
}


# Wanted a custom formatter to leave empty keys to be filled in later. Python2 needs this
# https://stackoverflow.com/questions/17215400/python-format-string-unused-named-arguments
class MyFormatter(string.Formatter):
    def __init__(self, default='{{{0}}}'):
        self.default=default

    def get_value(self, key, args, kwds):
        if isinstance(key, str):
            return kwds.get(key, self.default.format(key))
        else:
            return string.Formatter.get_value(key, args, kwds)


fmt=MyFormatter()


base_format = '{start_char}{target_dest}{source_dest}{payload}{csum}{end_char}'
base_format = fmt.format(base_format, 
                         start_char=general_formatter["start_char"], 
                         end_char=general_formatter["end_char"],
                         csum=general_formatter["csum"])

base_pattern = '{start_char}{target_dest}{source_dest}{payload}{csum}{end_char}'
base_pattern = fmt.format(base_pattern,
                          start_char=general_formatter["start_char"], 
                          end_char=general_formatter["end_char"],
                          csum=general_formatter["csum"])

base_pattern_allid = fmt.format(base_pattern,
                                target_dest="([%s-%s])"%(cv_device_limits['min_destination'],
                                                        cv_device_limits['max_destination']),
                                source_dest="([%s-%s])"%(cv_device_limits['min_source_addr'],
                                                        cv_device_limits['max_source_addr']),     
)                           

pattern_setter_payload_limits = {
    "set_address": "[%s-%s]"%(cv_device_limits["min_seat_number"],
                              cv_device_limits["max_seat_number"]),
}
pattern_getter_payload_limits = {
    "get_address": "",
    "get_lock_format": "",
}
pattern_response_payload_limits = {
    "get_address": "(%s)"%pattern_setter_payload_limits["set_address"],
    "get_lock_format": "(%s)(%s)(%s)"%(cv_device_limits["video_formats"],
                                   cv_device_limits["force_auto"],
                                   cv_device_limits["lock_unlock"]),
}


pattern_setters = {} 
for pattern_name in pattern_setter_payload_limits:
    pattern_setters[pattern_name] = fmt.format(base_pattern_allid,
                                    payload = command_identifier_dict[pattern_name] +
                                              pattern_setter_payload_limits[pattern_name]
                                    )

pattern_getters = {} 
for pattern_name in pattern_getter_payload_limits:
    pattern_getters[pattern_name] = fmt.format(base_pattern_allid,
                                    payload = command_identifier_dict[pattern_name] + 
                                        pattern_getter_payload_limits[pattern_name]
                                    )


pattern_responses = {}
for pattern_name in pattern_response_payload_limits:
    pattern_responses[pattern_name] = fmt.format(base_pattern_allid,
                                    payload = response_identifier_dict[pattern_name] + 
                                        pattern_response_payload_limits[pattern_name]
                                    )


# pattern_setters:
# #     "set_antenna_mode": r'\n([0-9])([0-9])AN([0-3])!\r',
# #     "set_channel":  r'\n([0-9])([0-9])BC([0-7])!\r',
# #     "set_band": r'\n([0-9])([0-9])BG([1-8,a-f])!\r',
# #     "set_video_mode": r'\n([0-9])([0-9])MD([L,S,M])!\r',
# #     "set_osd_visibility": r'\n([0-9])([0-9])OD([E,D])!\r',
# #     "set_osd_position": r'\n([0-9])([0-9])ODP([0-7])!\r',
# #     "set_id":  r'\n([0-9])([0-9])ID(.{0,12})!\r',
# #     "reset_lock": r'\n([0-9])([0-9])RL!\r',
# #     "set_video_format": r'\n([0-9])([0-9])VF([NAP])!\r',
# # }

# pattern_getters:

#     "get_address": r'\n([0-9])([0-9])RPAD!\r', #TODO here and below
#     "get_channel": r'\n([0-9])([0-9])RPBC!\r',
#     "get_band": r'\n([0-9])([0-9])RPBG!\r',
#     "get_frequency": r'\n([0-9])([0-9])RPFR!\r',
#     "get_osd_string": r'\n([0-9])([0-9])RPID!\r',
#     "get_lock_format": r'\n([0-9])([0-9])RPLF!\r',
#     "get_mode": r'\n([0-9])([0-9])RPMD!\r',
#     "get_model_version": r'\n([0-9])([0-9])RPMV!\r',
#     "get_rssi": r'\n([0-9])([0-9])RPRS!\r',
#     "get_osd_state": r'\n([0-9])([0-9])RPOD!\r',
#     "get_video_format": r'\n([0-9])([0-9])RPVF!\r',

# # The last part of the matched tuples is the data field that changes in the cv receiver
matched_command_tuples = {
    "set_address": namedtuple('set_address', ['rx_address',
                                            'requestor_id',
                                            'address']),
    "set_antenna_mode": namedtuple('set_antenna_mode', ['rx_address',
                                                        'requestor_id',
                                                        'antenna_mode']),
    "set_channel": namedtuple('set_channel', ['rx_address',
                                            'requestor_id',
                                            'channel']),
    "set_band": namedtuple('set_band', ['rx_address',
                                        'requestor_id',
                                        'band']),
    "set_video_mode": namedtuple('set_video_mode', ['rx_address',
                                                    'requestor_id',
                                                    'video_mode']),
    "set_osd_visibility": namedtuple('set_osd_visibility', ['rx_address',
                                                            'requestor_id',
                                                            'osd_enable']),
    "set_osd_position": namedtuple('set_osd_position', ['rx_address',
                                                        'requestor_id',
                                                        'osd_position']),
    "set_id": namedtuple('set_id', ['rx_address',
                                    'requestor_id',
                                    'id']),
    "reset_lock": namedtuple('reset_lock', ['rx_address',
                                            'requestor_id']),
    "set_video_format": namedtuple('video_format', ['rx_address',
                                                    'requestor_id',
                                                    'video_format']),
}

matched_report_tuples = {
    #TODO these need the remaining fields filled in
    "get_address": namedtuple('set_address', ['rx_address',
                                            'requestor_id']),
    "get_channel": namedtuple('set_address', ['rx_address',
                                            'requestor_id']),
    "get_band": namedtuple('set_address', ['rx_address',
                                        'requestor_id']),
    "get_frequency": namedtuple('set_address', ['rx_address',
                                                'requestor_id']),
    "get_osd_string": namedtuple('set_address', ['rx_address',
                                                'requestor_id']),
    "get_lock_format": namedtuple('set_address', ['rx_address',
                                                'requestor_id']),
    "get_mode": namedtuple('set_address', ['rx_address',
                                        'requestor_id']),
    "get_model_version": namedtuple('set_address', ['rx_address',
                                                    'requestor_id']),
    "get_rssi": namedtuple('set_address', ['rx_address',
                                        'requestor_id']),
    "get_osd_state": namedtuple('set_address', ['rx_address',
                                                'requestor_id']),
    "get_video_format": namedtuple('set_address', ['rx_address',
                                                'requestor_id']),
}

matched_response_tuples = {
    "get_lock_format" :namedtuple('lock_format_report', ['requestor_id', 
                                                         'rx_address', 
                                                         'chosen_camera_type', 
                                                         'forced_or_auto', 
                                                         'locked_or_unlocked']),
}

# link the getter name to the data field name and report name
report_links = {
    #"get_address": ("address", command_identifier_dict["get_address"]),
    "get_channel": ("channel", "BC"), #TODO fill in the rest with command_identifier_dict
    "get_band": ("band","BG"),
    "get_frequency": ("frequency","FR"),
    "get_osd_string": ("id","ID"),
    "get_lock_format": ("lock_format","LF"),
    "get_mode": ("video_mode","MD"),
    "get_model_version": ("model_version","MV"),
    "get_rssi": ("rssi","RS"),
    "get_osd_state": ("osd_enable","OD"),
    "get_video_format": ("video_format","VF"),
}


def format_command(target_dest, source_dest, command, *args):
    """Returns a fully formatted command for CV. Accepts variable arguments to append to command.
    target_dest: Which receiver to target
    source_dest: Where the message came from
    command: Which CV supported command to use
    args = anything to append to command (not checked)

    This doesn't check the arguments for the command are in a range the CV expects
    """
    extra_args = ""

    # Perform checks on inputs. 
    if not check_target_dest(target_dest):
        return None
    if not check_source_dest(source_dest):
        return None
    if not check_command_identifier(command):
        return None

    args_str = []
    for arg in args:
        args_str.append(str(arg))

    extra_args = r''.join(args_str)
    
    # Passed all checks. Format the command
    return fmt.format(base_format, 
                      target_dest=target_dest, 
                      source_dest=source_dest,
                      payload=command + extra_args)

def create_command(target_dest, command_name, source_dest=9, *args):
    "Perform pre-checks for both getters and setters"
    try:
        command_identifier = command_identifier_dict[command_name]
    except KeyError:
        logger.error("command_name of '%s' not found in send_command"%command_name)

    # Check the variable argument limits
    
    return format_command(target_dest, source_dest, command_identifier, *args)





def check_target_dest(dest):
    if dest == clearview_specs["bc_id"] or \
        cv_device_limits["min_destination"] <= dest <= cv_device_limits["max_destination"]:
        return True
    else:
        logger.error("target_destination of '%s' out of range in check_target_dest"%dest)
        return False

def check_source_dest(dest):
    if cv_device_limits["min_source_addr"] <= dest <= cv_device_limits["max_source_addr"]:
        return True
    else:
        logger.error("source_destination of '%s' out of range in check_source_dest"%dest)
        return False

def check_command_identifier(command):
    if command not in command_identifier_set:
        logger.error("Command of '%s' not found in check_command_identifier"%command)
        return False
    else:
        return True

def create_setter_command(target_dest, command_name, source_dest=9, *args):
    """ Create a setter command to be sent to a clearview"""

    cmd = create_command(target_dest, command_name, source_dest, *args) 
    if cmd == None:
        return None
    
    pattern = pattern_setters[command_name]
    match = re.search(pattern, cmd)
    if match is None:
        logger.error("Setting of '%s' not possible because extra command parameter(s) in"
                        "'%s' are invalid. Attempted to match %s "%
                        (command_name,repr(cmd), repr(pattern)))
        return None
    else:
        return cmd



def create_getter_command(target_dest, command_name, source_dest=9, *args):
    """ Create a getter command to be sent to a clearview"""
    cmd = create_command(target_dest, command_name, source_dest, *args) 
    if cmd == None:
        return None
    
    pattern = pattern_getters[command_name]
    match = re.search(pattern, cmd)
    if match is None:
        logger.error("Getting of '%s' not possible because extra command parameter(s) in"
                        "'%s' are invalid. Attempted to match %s "%
                        (command_name,cmd.strip(), pattern))
        return None
    else:
        return cmd

def match_response(response):
    """Take any response from a ClearView and return the parsed response and command type"""

    for pattern_name, pattern in pattern_responses.items():
        
        match = re.search(pattern, response)
        
        if match:
            nt = matched_response_tuples[pattern_name]._make(match.groups())
            return nt, pattern_name
    print("Unable to match response %s to any patter"% repr(response))
    return None

def extract_data(nt, pattern_name):
    if pattern_name == "get_lock_format":
        return {"lock_status":nt.locked_or_unlocked,
                "cam_forced_or_auto":nt.forced_or_auto,
                "chosen_camera_type":nt.chosen_camera_type}
    else:
        logger.warning("No data has been defaulted to extract for %s"% pattern_name)
        return None

def main():
    print()
    print(create_setter_command(1, "set_address", 8, 4))
    nt, pname = match_response("\n91LFPAU%\r")
    print(extract_data(nt, pname))

    # glf = matched_response_tuples["get_lock_format"]
    # if isinstance(nt, glf):
    #     print("ISLOCK")
    # else:
    #     print("NOTYPE")
    #     print(type(nt))
    #     print(type(glf))
    #     print(nt.__dict__)
    #     print(glf.__name__)
    
    # print(create_getter_command(1, "get_lock_format", 8))
    # print(pattern_setters["set_address"])
    # print(pattern_setters["set_band"])
    pass

if __name__ == "__main__":
    main()