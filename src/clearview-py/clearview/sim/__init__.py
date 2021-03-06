try:
    import queue
except ImportError:
    import Queue as queue
import json
import atexit
import logging
from collections import namedtuple
import re
import clearview
from clearview.formatter import report_links, matched_command_tuples, matched_report_tuples
from clearview.formatter import pattern_setters as patterns #TODO patterns used to be for both setters and getters which is not efficient. May have bugs of unknown patterns

logger = logging.getLogger(__name__)

class ClearViewReceiver:
    def __init__(self, input_data=None, rx_addr=1):

        defaults = {
            "address": "1",
            "antenna_mode": "1",
            "channel": "0",
            "band": "2",
            "video_mode": "L",      # live
            "id": "ClearViewN",     # osd string
            "osd_enable": "E",      # enabled
            "osd_position": "0",    # top left
            "video_format": "N",   # NTSC
            "lock_format": None,
            "model_version": None,
            "rssi": None,


        }
        if len(defaults) != len(matched_command_tuples) + len(matched_report_tuples) != len(patterns):
            print("Error: ClearViewReceiver MISMATCH LENGTHS")

        if input_data is None:
            self.data = defaults
        else:
            self.data = input_data

        # TODO check intput_data keys vs defaults keys ")

        with open("ClearViewReceiverDefaults.txt", 'w') as outfile:
            json.dump(defaults, outfile, indent=4)


# keep track of which video channels are "turned on" and what the camera format is
class RfEnvironment:
    def __init__(self):

        self.logger = logger
        file_name = "cv_rf_settings.txt"

        def _close_save():
            self.logger.info("Saving RF settigns to %s", file_name)
            with open(file_name, 'w') as outfile:
                json.dump(self.frequencies, outfile)

        self._load_in_file(file_name)
        atexit.register(_close_save)

    def turn_on_channel(self, frequency, camera_mode):
        if camera_mode not in ['n', 'p']:
            self.logger.fatal("Unsupported channel camera_mode of %s ", camera_mode)
        else:
            self.frequencies[frequency] = camera_mode

    def turn_off_channel(self, frequency):
        try:
            del self.frequencies[frequency]
        except KeyError:
            self.logger.error("Frequency %s can't be deleted because it doesn't exist", frequency)

    def _load_in_file(self, file_name):
        try:
            with open(file_name) as json_file:
                self.frequencies = json.load(json_file)
        except FileNotFoundError:  # start with empty file
            self.logger.info("Starting with empty RF environement")
            self.frequencies = {
                5800: 'n',
                5880: 'p'
            }
            self.logger.info("Frequencies Loaded: %s", self.frequencies)


class ClearViewSerialSimulator:
    def __init__(self, sim_file_load_name=None, sim_file_save_name=None):
        self.port = "simulated_port"
        self._serial_buffer = queue.Queue()    # a list of messages to reply when asked for a read
        self.logger = logger
        self._sim_file_load_name = sim_file_load_name
        self._sim_file_save_name = sim_file_save_name

        self._rf_environment = RfEnvironment()

        self.logger.setLevel(logging.INFO)

        # Save the serial communication protocol to the class
        self.msg_start_char = clearview_specs['message_start_char']
        self.msg_end_char = clearview_specs['message_end_char']
        self.mess_src = clearview_specs['mess_src']
        self.csum = clearview_specs['message_csum']
        self.bc_id = clearview_specs['bc_id']

        def _close_save():
            if sim_file_save_name is not None:
                self.logger.info("Saving settigns to %s",
                                 sim_file_save_name)
                with open(sim_file_save_name, 'w') as outfile:
                    data_to_dump = {}
                    for r in self.receivers:
                        data_to_dump[r] = self.receivers[r].data
                    json.dump(data_to_dump, outfile, indent=4)
            else:
                self.logger.info("Not saving cv_settings ")

        self._load_in_file()
        atexit.register(_close_save)

    # writing a message to simulator means it needs to be parsed.
    # If the msg is a report request and the cv exists, add the response data to the _serial_buffer

    def write(self, msg):
        msg = msg.decode('UTF8')
        self._parse_message(msg)

    # read from the serial means putting serial
    def read(self, msg, timeout=None):
        return self._serial_buffer.get(timeout=timeout)

    def read_until(self, terminator=None):
        if terminator is None:
            terminator = self.msg_end_char

        msg = ""
        while True:
            # https://docs.python.org/2/library/queue.html#Queue.Queue.get
            # Will generate empty exception. so return empty string?
            try:
                r = self._serial_buffer.get(block=False)
            except queue.Empty:
                self.logger.info("Error. No data in buffer that ends in termchar. current data is %s", msg)
                return "".encode()   # No termchar to read

            msg += r    # TODO this is horribly slow
            if r == terminator:
                return msg.encode()

        raise Exception("Error. Reached end of queue without generating error")

    def _parse_message(self, msg):
        self.logger.debug("Attempt to parse %s", msg)

        # Address

        for pattern in patterns:
            self.logger.debug("Attempting to match on pattern: %s", pattern)

            try:
                match = re.search(patterns[pattern], msg)
            except:
                pass

            if match:
                if pattern in matched_command_tuples.keys():
                    nt = matched_command_tuples[pattern]._make(match.groups())
                    self._change_cv_data(nt)
                    return True
                elif pattern in matched_report_tuples.keys():
                    nt = matched_report_tuples[pattern]._make(match.groups())
                    # Need to pass in pattern because the pattern is not accessible
                    # from the named tuple
                    self._report_cv_data(nt, pattern)   
                    return True
                else:
                    self.logger.critical("No namedtuple for pattern of %s", pattern)
                    input("Press any key to acknowledge")
                    raise NotImplementedError("_parse_message: No namedtuple for pattern of " + pattern)
            else:
                self.logger.debug("_parse_message: No success matching  %s on pattern %s", msg.strip(), pattern)
        self.logger.critical("_parse_message: Unable to match %s on any pattern.", msg.strip())
        input("Press any key to acknowledge")
        raise NotImplementedError("_parse_message: Unable to match " + msg.strip() + " on any pattern")

    def _change_cv_data(self, nt):
        # TODO if rx_addr is 0, change all receivers
        # TODO if change of rx address is requested, need to move dictionary around AND edit field

        if len(nt._fields) == 2:
            # no action to take
            self.logger.info("No action to take on %s", nt)
            return

        rx_addr = nt.rx_address
        parameter_name = nt._fields[-1]
        new_value = nt[-1]
        new_value = str(new_value)
        old_rx_params = self.receivers[str(rx_addr)].data
        old_value = old_rx_params[parameter_name]
        logger.info("Changing rx#%s's parameter of %s from %s to %s", rx_addr, parameter_name, old_value, new_value)

        self.receivers[str(rx_addr)].data[parameter_name] = new_value

    def _report_cv_data(self, nt, pattern):
        # TODO if rx_ddr is 0, generate an warning that it is bad practice to request report from all

        rx_addr = nt.rx_address
        parameter_name = report_links[pattern][0]
        message_identifier = report_links[pattern][1]

        try:
            cv_of_interest = self.receivers[rx_addr]

            try:
                cvdata_of_interest = cv_of_interest.data[parameter_name]
            except KeyError:
                self.logger.critical("Receiver %s does not have data object %s", rx_addr, parameter_name)
                quit()
        except KeyError:
            self.logger.critical("The receiver ID of %s does not not exist in simulated receivers", rx_addr)
            quit()

        logger.info("Reporting rx#%s's pattern/param of %s/%s. Value is %s",
                    rx_addr,
                    pattern,
                    parameter_name,
                    cvdata_of_interest)

        # TODO check the rx_address and msg_source are actually correct here. 
        # For example, the mess_src parameter should actually be the receiver id, and the source 
        # would actually be the requester id. 

        report_message = self._format_write_command(nt.requestor_id, nt.rx_address, message_identifier + cvdata_of_interest)

        # Put the characters in one at a time
        for c in report_message:
            self._serial_buffer.put(c)

    def _format_write_command(self, requestor_target, receiver_src, message):   # formats sending commands with starting chars, addresses, csum, message and ending char. Arguments may be strings or ints
        return self.msg_start_char + str(requestor_target) + str(receiver_src) + str(message) + self.csum + self.msg_end_char

    def _load_in_file(self):
        if self._sim_file_load_name is not None:
            try:
                with open(self._sim_file_load_name) as json_file:
                    loaded_data = json.load(json_file)

            except FileNotFoundError as f:  # start with empty file
                raise f
        else:
            self.logger.info("Starting with empty cv data")
            loaded_data = {}

        self.receivers = {}
        for k in range(8):
            if str(k+1) in loaded_data.keys():
                loaded_rx_data = loaded_data[str(k+1)]
                self.logger.info("_load_in_file:  Using loaded data for %s ", k+1)
                self.logger.debug("_load_in_file: Loaded rx data: %s", loaded_rx_data)

                self.receivers[str(k+1)] = ClearViewReceiver(input_data=loaded_rx_data)
            else:
                self.logger.info("_load_in_file: Using defaults for %s", k+1)
                self.receivers[str(k+1)] = ClearViewReceiver()
                # self.receivers = {}
                #     for i in range(8):
                #         self.receivers[str(i+1)] = ClearViewReceiver()
                #     self._data = json.load(json_file)