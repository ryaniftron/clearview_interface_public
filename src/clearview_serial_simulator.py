import queue
import json
import atexit
import logging
from collections import namedtuple
import re

logger = logging.getLogger(__name__)

patterns = {
    "set_address": r'\n([0-9])([0-9])AD([1-8,a-f])!\r',
    "set_band": r'\n([0-9])([0-9])BC([1-8,a-f])!\r',
}

matched_tuples = {
    "get_address": namedtuple('address_report', ['requestor_id', 'rx_address', 'replied_address']),
    "set_band": namedtuple('band_group_report', ['requestor_id', 'rx_address', 'band_index']),
}


class ClearViewReceiver:
    def __init__(self, input_data=None, rx_addr=1):

        defaults = {
            "band":"X",
            "channel":"0",
            "address":"1",
            "antenna_mode":"1",
            "video_mode":"n",
            "id":"ClearView"
        }

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
            self.logger.info("Frequencies Loaded: ", self.frequencies)


class ClearViewSerialSimulator:
    def __init__(self, sim_file_load_name=None, sim_file_save_name=None):
        self.port = "simulated_port"
        self._serial_buffer = queue.Queue()    # a list of messages to reply when asked for a read
        self.logger = logger
        self._sim_file_load_name = sim_file_load_name
        self._sim_file_save_name = sim_file_save_name

        self._rf_environment = RfEnvironment()

        self.logger.setLevel(logging.INFO)

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

    def read_until(self, *, terminator='\r'):
        msg = ""
        while True:
            # https://docs.python.org/2/library/queue.html#Queue.Queue.get
            # Will generate empty exception. so return empty string?
            try:
                r = self._serial_buffer.get(block=False)
            except queue.Empty:
                self.logger.info("Error. No data in buffer that ends in termchar. current data is %s",msg)
                return "".encode()   # No termchar to read

            msg.append(r)
            if r == terminator:
                return msg.encode()

        raise Exception("Error. Reached end of queue without generating error")

    def _parse_message(self, msg):
        self.logger.info("Attempt to parse %s", msg)

        # Address

        for pattern in patterns:
            print("Attempting to match on pattern:", pattern)
            match = re.search(patterns[pattern], msg)
            
            if match:
                if pattern in matched_tuples.keys():
                    nt = matched_tuples[pattern]._make(match.groups())
                    print("Success: Do something with nt:", nt)
                    self._change_cv_data(nt)
                else:
                    self.logger.critical("No namedtuple for pattern of %s", pattern)
            else:
                print("Fail: no match")

    def _change_cv_data(self,nt):
        
        #TODO if rx_addr is 0, change all receivers

        # no limit checking. This was already done in ClearView.py

        value_field = nt._fields[-1]
        new_value = nt[1]
        new_value = str(new_value)
        old_value = self.receivers[str(rx_addr)][parameter_name]
        logger.info("Changing rx addr %s's parameter of %s from %s to %s", rx_addr,parameter_name,old_value,new_value)
        self.receivers[str(rx_addr)][parameter_name]= new_value
        

        # OSD String

    def _load_in_file(self):
        if self._sim_file_load_name is not None:
            try:
                with open(self._sim_file_load_name) as json_file:
                    loaded_data = json.load(json_file)
                    print("Loaded data successfully.")

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

