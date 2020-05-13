# ClearView.py
# Author: Ryan Friedman
# Date Created: Feb 3,2020
# License: GPLv3

# Change Log


# 2020-02-03
# Command Protocol 1.20
# https://docs.google.com/document/d/1oVvlSCK49g7bpSY696yi9Eg0pA1uoW88IxbgielnqIs/edit?usp=sharing

# Initial Release of v1.20.0 for CV1.20

import serial
from time import sleep
import inspect
import sys  # for printing caller in debug
import re
from collections import namedtuple
import logging
from . import sim
from . import comspecs
from . import formatter

clearview_specs = comspecs.clearview_specs

__version__ = '1.20a1'

logger = logging.getLogger(__name__)


# try:
#     import comspecs
#     clearview_specs = comspecs.clearview_specs
# except ImportError:
#     print("No clearview_specs. Using defaults")
#     baudrate = 19200
#     clearview_specs = {
#         'message_start_char': '\n',
#         'message_end_char': '\r',
#         'message_csum': '%',
#         'mess_src': 9,
#         'baud': 57600,
#         'bc_id': 0
#     }

# TODO the docstring seems to be in alphabetical order but it keeps headers in weird
# TODO make all arguments named args
# TODO perhaps all the setters can be sent in as a list
# TODO check all logging and print statements use self.logging


class ClearView:
    """Communicate with a ClearView using a serial port"""
    def __init__(self, timeout=0.5, 
                 debug=False, 
                 simulate_serial_port=False,
                 sim_file_load_name=None,
                 sim_file_save_name=None,
                 return_formatted_commands=False,
                 port='/dev/ttyS0', 
                 robust=True):

        # Pick whether to simulate ClearViews on the serial port or not
        self.simulate_serial_port = simulate_serial_port
        

        if self.simulate_serial_port:
            self._serial = sim.ClearViewSerialSimulator(
                            sim_file_load_name=sim_file_load_name,
                            sim_file_save_name=sim_file_save_name
            )
        elif return_formatted_commands==True:
            # Just use the module to generate commands based on the serial specs
            self._serial = None
        else:
            self._serial = serial.Serial(
                port=port,
                baudrate=clearview_specs['baud'],
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=timeout   # number of seconds for read timeout
            )

        self.msg_start_char = clearview_specs['message_start_char']
        self.msg_end_char = clearview_specs['message_end_char']
        self.mess_src = clearview_specs['mess_src']
        self.csum = clearview_specs['message_csum']
        self.bc_id = clearview_specs['bc_id']

        # debug => Used for printing all serial comms and warnings. Critical Error messages are printed no debug_msg state.
        self.debug_msg = debug
        # self.print only prints data if self.debug_msg is True
        self._print("CV Debug Messages Enabled")

        # robust mode => Slower, but all setters are automatically checked with a get
        self.robust = robust
        # number of control command retries before giving up
        self.robust_control_retries = 3
        # number of report request retries before giving up
        self.robust_report_retries = 1

        if self._serial is not None:
            self._print("Connecting to ", self._serial.port, sep='\t')

        # TODO get the logger working instead of _print
        self.logger = logger

    # ###########################
    # ### CV Control Commands ###
    # ###########################
    def set_address(self, rcvr_target, new_target ,robust=False):
        """ADS => set_address => rcvr_target is the receiver
        seat number of interest,0-8, 0 for broadcast.
        new_target is the new seat 1-8

        Robust mode can be turned off here if desired otherwise
        scanning for connected receivers is painfully slow.

        Return:
            Success: True
            Fail to Set: False
            Error in command: -1

        #TODO bug: When target is 0, this fails to report success even if it was
        """

        num_tries = self._get_robust_retries(mode="send", robust=robust)

        
        cmd = self._format_setter_command("set_address",
                                          target_dest=rcvr_target, 
                                          extra_params = (new_target,))
        if self._serial is None:
            return cmd

        while num_tries > 0:
            self._write_serial(cmd)

            # If successful, expect this to NOT return an address because the receiver
            # is no longer at that address. This does not guarantee success because the
            # receiver could not be exist on either the old or new channel
            if robust:
                rx_no_longer_old_addr = not (self.get_address(rcvr_target=rcvr_target) is None)

                # If successful, expect this to return an address. This is the more important
                # test of success
                rx_is_now_new_addr = (self.get_address(rcvr_target=new_target) is not None)

                if rx_no_longer_old_addr and rx_is_now_new_addr:
                    return True
                else:
                    self.logger.warning("Failed set_address with %s tries left", num_tries)
                    num_tries = num_tries - 1
            else:
                return True     # was in robust mode. assume it worked
            


    def set_antenna_mode(self, rcvr_target, antenna_mode):
        """AN => Set antenna mode between legacy, L,R,CV

        Robust Mode: Not supported
        """

        cmd = self._format_setter_command("set_antenna_mode",
                                            target_dest=rcvr_target, 
                                            extra_params = (antenna_mode,))
        return self._write_serial(cmd)
   

    def set_band_channel(self, rcvr_target, band_channel, robust = False):
        """BC = > Sets band channel. 0-7"""

        num_tries = self._get_robust_retries(mode="send", robust=robust)


        while num_tries > 0:
            sleep(0.25)
            self._clear_serial_in_buffer()
            cmd = self._format_setter_command("set_channel",
                                              target_dest=rcvr_target, 
                                              extra_params = (band_channel,))
            
            if self._serial is None:
                return cmd

            self._write_serial(cmd)
            sleep(0.25)
            if robust:
                bc = self.get_band_channel(rcvr_target=rcvr_target)
                if bc is not None:
                    if bc.channel == band_channel:
                        return True
                else:
                    self.logger.warning("Failed set_band_channel with %s tries left", num_tries)
                num_tries = num_tries - 1
            else:
                return True     # not robust, no need query. just return true
        return False

        

    def set_band_group(self, rcvr_target, band_group):
        """BG = > Sets band. 0-9,a-f"""
        cmd = self._format_setter_command("set_band",
                                           target_dest=rcvr_target, 
                                           extra_params = (band_group,))
        return self._write_serial(cmd)

    def set_custom_frequency(self, rcvr_target, frequency):
        """#FR = > Sets frequency of receiver by finding band and channel"""
        # This command does not yet work in ClearView v1.20
        
        min_freq = comspecs.cv_device_limits["min_frequency"]
        max_freq = comspecs.cv_device_limits["max_frequency"]

        if min_freq <= frequency <= max_freq:

            #because setting direct frequency not supported, set band and channel
            band_channel = comspecs.frequency_to_bandchannel(frequency)
            if band_channel is not None:
                band_letter = band_channel[0]
                channel_num = int(band_channel[1])-1
                cv_band = comspecs.band_name_to_cv_index(band_letter)

                #channel number
                channel_command = self.set_band_channel(rcvr_target=rcvr_target,
                                                        band_channel=channel_num)                
                band_command = self.set_band_group(rcvr_target=rcvr_target,
                                                   band_group=cv_band)
            else:

                # TODO when CV supports any frequency, not just in the band/channel charts, just have this throw
                self.logger.error("Custom Frequency of %s is not supported" % frequency)
                raise NotImplementedError

            return [channel_command, band_command]
        else:
            self.logger.error("Frequency %s out of range of (%s,%s)"%(frequency, min_freq, max_freq))
            raise ValueError


    def set_temporary_frequency(self, rcvr_target, frequency):
        """TFR = > Sets frequency of receiver and creates custom frequency if needed"""
        # This command does not yet work in ClearView v1.20
        min_freq = clearview_specs

        raise NotImplementedError

    def set_video_mode(self, rcvr_target, mode):
        """#MDL,MDS,MDM show live video, spectrum analyzer, and user menu

            mode = "live", "spectrum", "menu"
        """

        avail_modes = ["live", "spectrum", "menu"]
        if mode in avail_modes:
            if mode == "live":
                modeK = 'L'
            elif mode == "spectrum":
                modeK = 'S'
            elif mode == "menu":
                modeK = 'M'
            else:
                raise Exception
            cmd = self._format_setter_command("set_video_mode",
                                              target_dest=rcvr_target, 
                                              extra_params = (modeK,))
            return self._write_serial(cmd)
        else:
            self.logger.error("Unsupported video mode of %s"%mode)

    def show_osd(self, rcvr_target):
        """ ODE shows/enables the osd"""

        cmd = self._format_setter_command("set_osd_visibility",
                                          target_dest=rcvr_target, 
                                          extra_params = ("E",))   # Enable
        return self._write_serial(cmd)

    def hide_osd(self, rcvr_target):
        """ODD hides/disables the osd"""

        cmd = self._format_setter_command("set_osd_visibility",
                                    target_dest=rcvr_target, 
                                    extra_params = ("D",))    # Disable
        return self._write_serial(cmd)

    def set_osd_at_predefined_position(self, rcvr_target, desired_position):
        """Set OSD at integer position, 0-7"""
        
        cmd = self._format_setter_command("set_osd_position",
                                target_dest=rcvr_target, 
                                extra_params = (desired_position,)) 
        return self._write_serial(cmd)

    def set_osd_string(self, rcvr_target, osd_str):
        """ ID => Sets OSD string"""
        osd_str_cut = osd_str[:comspecs.cv_device_limits['max_id_length']]

        if osd_str_cut != osd_str:
            self.logger.warning("Truncating osd_string from '%s' to '%s'"%(osd_str,osd_str_cut))
        
        cmd = self._format_setter_command("set_id",
                                target_dest=rcvr_target, 
                                extra_params = (osd_str_cut,)) 
        return self._write_serial(cmd)

    def set_osd_string_positional(self, rcvr_target, starting_index, osd_str):
        """ IDP => Sets OSD positional string, starting at a certain character in the OSD"""
        raise NotImplementedError
        osd_str_max_len = 4

        print("Before cut: ", osd_str, "with supplied length of ", len(osd_str), " and max total length of ", osd_str_max_len)
        # Cut the string length down
        char_count_avail = osd_str_max_len - starting_index
        osd_str = osd_str[:char_count_avail]
        print("After cut: ", osd_str)


        old_osd_string = ''.join([char for char in '*'])
        new_osd_string = new_osd_string.replace(old_osd_string,new_osd_string)
        
  

    def reboot(self, rcvr_target):
        """RBR => Reboot receiver. Issue twice to take effect."""
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError


        # self._write_serial(cmd)
        # sleep(0.1)
        # self._write_serial(cmd)

    def reset_lock(self, rcvr_target):
        """RL => Reset Lock """

        cmd = self._format_setter_command("reset_lock",
                                target_dest=rcvr_target)   
        return self._write_serial(cmd)

    def set_video_format(self, rcvr_target, video_format):
        """VF => Temporary Set video format.
        video_format = [N or NTSC , P or PAL , A or AUTO]
        """

        desired_video_format = video_format.lower()
        if desired_video_format == "n" or desired_video_format == "ntsc":
            vf = 'N'
        elif desired_video_format == "p" or desired_video_format == "pal":
            vf = 'P'
        elif desired_video_format == "a" or desired_video_format == "auto":
            vf = 'A'
        else:
            self.logger.error("Error. Invalid video format in set_temporary_video_format of ", desired_video_format)
            return None

        cmd = self._format_setter_command("set_video_format",
                        target_dest=rcvr_target, 
                        extra_params = (vf,)) 
        return self._write_serial(cmd)

    def move_cursor(self, rcvr_target, desired_move):
        """ WP => Move cursor [+-EPMX]"""
        cmd = self._format_setter_command("move_cursor",
                        target_dest=rcvr_target, 
                        extra_params = (desired_move,)) 
        return self._write_serial(cmd)

    def set_temporary_osd_string(self, rcvr_target, osd_str):
        """  TID => Temporary set OSD string"""
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # cmd = TODO
        # self._write_serial(cmd)

    def set_temporary_video_format(self, rcvr_target, video_format):
        """TVF => Temporary Set video format. options are 'N' (ntsc),'P' (pal),'A' (auto)"""
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # desired_video_format = video_format.lower()
        # if desired_video_format == "n" or desired_video_format == "ntsc":
        #     
        #     self._write_serial(cmd)
        # elif desired_video_format == "p" or desired_video_format == "pal":
        #    
        #     self._write_serial(cmd)
        # elif desired_video_format == "a" or desired_video_format == "auto":
        #     
        #     self._write_serial(cmd)
        # else:
        #     print("Error. Invalid video format in set_temporary_video_format of ",desired_video_format)

    def set_user_message(self, rcvr_target, osd_str):
        """UM => Sets user message string

        Query: No query available
        """
        osd_str_cut = osd_str[:comspecs.cv_device_limits['max_um_length']]

        if osd_str_cut != osd_str:
            self.logger.warning("Truncating user message string from '%s' to '%s'"%(osd_str,osd_str_cut))
        
        cmd = self._format_setter_command("set_user_message",
                                target_dest=rcvr_target, 
                                extra_params = (osd_str_cut,))

        return self._write_serial(cmd)

    def send_report_cstm(self, rcvr_target, custom_command=None):
        """ #RP XX => Custom report"""
        
        # TODO. This knows the expected format.
        # Add a parameter to try parsing the return value
        # It knows which report it's expecting

        while True:
            #self._clear_serial_in_buffer()
            if custom_command is not None:
                cmd = TODO + custom_command
            else:
                cmd = TODO +  input('Enter a command')
            self._write_serial(cmd)
            # sleep(0.05)
            report = self._read_until_termchar()
            print(report)

    ############################
    #       Broadcast Messages #
    ############################

    def set_all_receiver_addresses(self, new_address):
        self.set_address(self.bc_id, new_address)

    def set_all_osd_message(self, message):
        self.set_osd_string(self.bc_id, message)

    # ##########################
    # ### CV Report Commands ###
    # ##########################

    """
    All report commands start with "get". Each of them sends the report command to a receiver, waits a bit for a response,
    and parses the response into a namedtuple.

    The report commands below use a regex to match the response based on the expected input.

    Reports return None if the CV is unresponsive or if the report doesn't match the expected report.

    TODO there could be improvements to the report error detection to log what is wrong with the regex match
    TODO the requestor_id should match clearview_comspecs.clearview_specs.mess_src 
    TODO the rx_address should match the sent rcvr_target

    Note: Reports are not intended to be requested of multiple receivers at the same time using broadcast.
        The only situation this makes sense is if you don't know the address.

        To find an unknown address, the following options exist
            1. use set_address(0,my_new_address). Ensure only one unit is connected otherwise they all get set to my_new_address
            2. use get_connected_receiver_list to see what the id is without changing it and update the known receiver address
            3. use get_address(0) to find the address. Ensure only one unit is connected

    """

    def get_address(self, rcvr_target):
        """RPAD => Get the address of a unit if it's connected. Only useful to see what units are connected to the bus
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_address"
        return self._run_report(rcvr_target, report_name)

    def get_channel(self, rcvr_target):
        """RPBC => Get the channel number
        """

        # TODO do I try to recatch serial exceptions here?
        report_name = "get_channel"
        return self._run_report(rcvr_target, report_name)

    def get_band(self, rcvr_target):
        """RPBG => Get the band group
        Returns namedtuple of receiver band with fields requestor_id, rx_address, and band_index

        TODO convert from band index to an actual band name using the maps in clearview_comspecs.
        Change variable from band_index to band_name"""

        # TODO do I try to recatch serial exceptions here?
        report_name = "get_band"
        return self._run_report(rcvr_target, report_name)

    def get_frequency(self, rcvr_target):
        """RPFR => Get the frequency
        """

        # TODO do I try to recatch serial exceptions here?
        report_name = "get_frequency"
        return self._run_report(rcvr_target, report_name)

    def get_osd_string(self, rcvr_target):
        """ RPID => Get the osd string, also called the "ID". Not to be confused with receiver address.
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_osd_string"
        return self._run_report(rcvr_target, report_name)

    def get_lock_format(self, rcvr_target):
        """ RPLF => Get the lock format. Used to tell if receiver is locked to the video signal
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_lock_format"
        return self._run_report(rcvr_target, report_name)

    def get_mode(self, rcvr_target):
        """ RPMD => Get the mode : live, menu, or spectrum analyzer
        Returns a namedtuple of receiver mode with fields requestor_id, rx_address, mode
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_mode"
        return self._run_report(rcvr_target, report_name)

    def get_model_version(self, rcvr_target):
        """ RPMV => Get the model version for hardware and software versions
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_model_version"
        return self._run_report(rcvr_target, report_name)

    def get_rssi(self, rcvr_target):
        """ RPRS => Get the rssi of each antenna
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_rssi"
        return self._run_report(rcvr_target, report_name)

    def get_osd_state(self, rcvr_target):
        """ RPOD => Get if OSD is enabled or disabled
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_osd_state"
        return self._run_report(rcvr_target, report_name) 

    def get_video_format(self, rcvr_target):
        """ RPVF => Get Video Format (pal,ntsc,auto)
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "get_video_format"

        return self._run_report(rcvr_target, report_name)


    def get_report_cstm_by_input(self, rcvr_target): #RP XX => Custom report
        while True:
            self._clear_serial_in_buffer()
            raise NotImplementedError
            #,"RP" + input('Enter a report type:'))
            self._write_serial(cmd)
            sleep(0.05)
            report = self._read_until_termchar()

            return report

    #Use this to see which device ID's are connected
    def get_connected_receiver_list(self):
        connected_receivers = {}
        for i in range(1,9):
            self._clear_serial_in_buffer()

            if self.get_address(rcvr_target=i) is not None:
                connected_receivers[i] = True
            else:
                connected_receivers[i] = False
            sleep(0.1)
        return connected_receivers

    def _get_robust_retries(self, mode, robust):
        if robust == False:
            return 1
        if mode == "send":
            return self.robust_control_retries
        elif mode == "receive":
            return self.robust_report_retries
        else:
            self.logger.critical("Unsuported mode of <percent>s for _get_robust_retries",stack_info=True)
            raise ValueError("Unsupported mode for robust retires")

    def _check_within_range(self, val, min, max):
        """Return True if value is within range,inclusive"""

        #TODO type check here
        try:
            if min is not None:
                if val < min:
                    return False    # Out of range
        except TypeError:
            self.logger.critical("Incorrect type for val or min in _check_within_range")
            return -1

        try:
            if max is not None:
                if val > max:
                    return False    # Out of range
        except TypeError:
            self.logger.critical("Incorrect type for val or min in _check_within_range")
            return -1
        return True                 # Success

    def _run_report(self, rcvr_target, report_name):
        """ 
        _run_report is hidden. It's to be used to send reports and parse responses
        
        Inputs:
            rcvr_target(int) = the receiver id
            report_name(string) =  Report command like "RPID"
            pattern (regex) = Regular Expression Response Parser
            reply_named_tuple (named_tuple) = Capture expected response in a tuple

        Outputs:
            named_tuple on success
            None  on failure

        Exception:
            TODO
        """
        cmd = self._format_report_request(report_name,
                                          target_dest=rcvr_target)
        
        if self._serial is None: # Return the formatted command
            return cmd

        raise NotImplementedError # The code below hasn't been updated with the new formatter
        # Because the expected reply is known based on the formatter, grab it here
        # If no reply, bail out

        # Otherwise, match the reply on a regex pattern from the new formatter
        #   If matched, form it into the nT and return it

        # if no match,try running the reply on all the possible replies
        #   if match, give warning that somethings' not setup but return nT
        # Otherwise,  return None with a warning that the reply isn't parsable
        
        self._clear_serial_in_buffer()
        
        self._write_serial(cmd)
        sleep(0.05)

        #python3

        #if (report := self._read_until_termchar()) is not None:
        report == self._read_until_termchar()
        if report is not None:
            # Try matching based on expected behavior
            match = re.search(pattern,report)
            
            if match:
                report = reply_named_tuple._make(match.groups())
            else:
                self.logger.error("When reporting %s from unit id#%s, the report of %s was not parsable.", report_name , rcvr_target, report.strip())
                report = None
        else:
            self.logger.error("When reporting %s from unit id#%s, there was no response", report_name , rcvr_target)
        sleep(0.05)
        return report

    

    # ########################### 
    # ### CV Serial Utilities ### 
    # ###########################

    def _send_custom_command(self,rcvr_target,cstm_cmd):
        cmd = self.TODO
        self._write_serial(cmd)

    def _read_serial_blocking(self,line_ending_char): #waits for data to come in as long as it takes
        rv = ""
        line_ending_byte = line_ending_char.encode() #converting string char to byte. Example. '\n' converts to b'\n' , '>' converts to b'>'
        while True: #read all available bytes till the line_ending_byte
            ch = self._serial.read() 
            if ch == line_ending_byte:
                return rv #ignore the line ending byte
            rv += ch.decode() #push the bytes onto a string by decoding and appending

    def _format_setter_command(self, 
                               command_name,
                               target_dest=clearview_specs["bc_id"],
                               source_dest=clearview_specs["mess_src"],
                               extra_params=()):
        return formatter.create_setter_command(
            command_name=command_name,
            target_dest=target_dest,
            source_dest=source_dest,
            extra_params=extra_params
        )

    def _format_report_request(self, 
                               command_name,
                               target_dest=clearview_specs["bc_id"],
                               source_dest=clearview_specs["mess_src"],
                               extra_params=()):
        return formatter.create_report_request(
            command_name=command_name,
            target_dest=target_dest,
            source_dest=source_dest,
            extra_params=extra_params
        )

    def _move_cursor(self,rcvr_target,direction):
        cmd = TODO
        self._write_serial(cmd)
    
    def _print(self,*args,**kwargs):
        if self.debug_msg==True:
            #python3            #print(args,sep=kwargs.get('sep','\t'))  
            print(args)  
    def _write_serial(self,msg):
        if self._serial is None: # Return the formatted command
            return msg

        self._print("CV_serial_write: ",
                    sys._getframe().f_back.f_back.f_code.co_name ,
                     "=>" ,
                     sys._getframe().f_back.f_code.co_name,
                     msg,
                     "cmd:",
                     msg,
                     **{'sep':' '})
        self._serial.write(msg.encode())
        # TODO should there be a return here?

    def _get_serial_in_waiting(self):
        try:
            in_wait = self._serial.in_waiting
            return in_wait
        except OSError:
            raise
        
    def _clear_serial_in_buffer(self):
        if self.simulate_serial_port:
            pass
        if self._serial is None:
            pass
        else:
            try:
                while self._get_serial_in_waiting() > 0:
                    self._serial.read()
                return True
            except OSError:
                raise

    def _read_until_termchar(self):
        if self._serial is None:
            return None

        str_read = self._serial.read_until(terminator = self.msg_end_char).decode('unicode_escape')
        if str_read == '':
            return None
        else:
            return str_read
        

if __name__ == "__main__": 
    print("This is ClearView module. Not main. Run main.py to understand how to use ClearView.py")
