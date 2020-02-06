### ClearView.py
### Author: Ryan Friedman
### Date Created: Feb 3,2020
### License: GPLv3

### Change Log


# 2020-02-03
# Command Protocol 1.20
# https://docs.google.com/document/d/1oVvlSCK49g7bpSY696yi9Eg0pA1uoW88IxbgielnqIs/edit?usp=sharing

# Initial Release of v1.20.0 for CV1.20



import serial
from time import sleep
import inspect,sys #for printing caller in debug
import re
from collections import namedtuple
import logging

__version__ = '1.20a1'

logger = logging.getLogger(__name__)


try:
    import clearview_comspecs
    clearview_specs = clearview_comspecs.clearview_specs
except ImportError:
    print("No clearview_specs. Using defaults")
    baudrate = 19200
    clearview_specs = {
        'message_start_char' : '\n',
        'message_end_char' : '\r',
        'message_csum' : '!',
        'mess_src'  : 9,
        'baud' : 57600,
    }



#TODO change all rx_id to rx_addr where appropriate because ID means the OSD string
#TODO the docstring seems to be in alphabetical order but it keeps headers in weird
    
class ClearView:
    """Communicate with a ClearView using a serial port"""
    def __init__(self,*args,**kwargs): 
        self._serial= serial.Serial(
           port = kwargs.get('port','/dev/ttyS0'), 
           baudrate = clearview_specs['baud'],
           parity = serial.PARITY_NONE,
           stopbits = serial.STOPBITS_ONE,
           bytesize = serial.EIGHTBITS,
           timeout = kwargs.get('timeout',0.5) # number of seconds for read timeout
           )
       
        self.msg_start_char = clearview_specs['message_start_char']
        self.msg_end_char = clearview_specs['message_end_char'] 
        self.mess_src = clearview_specs['mess_src']
        self.csum = clearview_specs['message_csum']
        
        #debug => Used for printing all serial comms and warnings. Critical Error messages are printed no debug_msg state.
        self.debug_msg = kwargs.get('debug',False)
        self._print("CV Debug Messages Enabled") #self.print only prints data if self.debug_msg is True. 
        
        #robust mode => Slower, but all setters are automatically checked with a get
        self.robust = kwargs.get('robust',False)
        self.robust_control_retries = 1 #number of control command retries before giving up
        self.robust_report_retries = 1 #number of report request retries before giving up
        
        self._print("Connecting to " , self._serial.port,sep='\t')
        self.logger = logger #TODO get the logger working instead of _print
        
    # ###########################   
    # ### CV Control Commands ###
    # ########################### 
    def set_receiver_address(self,rcvr_targets,new_target,*args,**kwargs): #ADS => Sets receiver address as new_target
        """set_receiver_address => rcvr_target is the receiver seat number of interest,0-8, 0 for broadcast. new_target is the new seat 1-8"""
        try: #rcvr_targets is a list of sorts
            for rx in rcvr_targets:
                if 0 <= rx <= 8 and 1 <= new_target <= 8:
                    cmd = self._format_write_command(rx,"ADS" + str(new_target))
                    self._write_serial(cmd)
                else:
                    print("Error: set_receiver_address. Receiver ", rx, "out of range")
        except: #rcvr_targets is an int or string
            if 0 <= rcvr_targets <= 8 and 1 <= new_target <= 8:
                cmd = self._format_write_command(int(rcvr_targets),"ADS" + str(new_target))
                self._write_serial(cmd) 
            else:
                print("Error: set_receiver_address. Receiver ", rcvr_targets, "out of range")

    def set_antenna_mode(self,rcvr_target,antenna_mode): #AN

        #TODO check range of rcvr_target and support list of rcvr_target
        if 0 <= antenna_mode <= 3:
            cmd = self._format_write_command(str(rcvr_target),"AN" + str(antenna_mode))
            self._write_serial(cmd)
        else:
            print("Error. Unsupported antenna mode of ", antenna_mode)


    def set_band_channel(self,rcvr_target,band_channel,*args,**kwargs): #BC = > Sets band channel. 1-8

        #TODO check range of rcvr_target and support list of rcvr_target
        if 1 <= band_channel <= 8:
            cmd = self._format_write_command(str(rcvr_target),"BC" + str(band_channel - 1))
            self._write_serial(cmd)
        else:
            print("Error. set_band_channel target of ",band_channel, "is out of range")

    def set_band_group(self,rcvr_target,band_group,*args,**kwargs): #BG = > Sets band. 0-9,a-f

        #TODO check range of rcvr_target and support list of rcvr_target
        #TODO add check if 0-9 and a-f
            cmd = self._format_write_command(str(rcvr_target),"BG" + str(band_group))
            self._write_serial(cmd)

    def set_custom_frequency(self,rcvr_target,frequency,*args,**kwargs): #FR = > Sets frequency of receiver and creates custom frequency if needed
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError
    

        # if 5200 <= frequency < 6000:
        #     cmd = self._format_write_command(str(rcvr_target),"FR" + str(frequency,))
        #     self._write_serial(cmd)
        # else:
        #     print("Error. Desired frequency out of range")

    def set_temporary_frequency(self,rcvr_target,frequency,*args,**kwargs): #TFR = > Sets frequency of receiver and creates custom frequency if needed
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError
        
        # if 5200 <= frequency < 6000:
        #     cmd = self._format_write_command(str(rcvr_target),"TFR" + str(frequency,))
        #     self._write_serial(cmd)
        # else:
        #     print("Error. Desired frequency out of range")

    def set_video_mode(self,rcvr_target,mode): #MDL,MDS,MDM

        #TODO check range of rcvr_target and support list of rcvr_target
        avail_modes = ["live","spectrum","menu"]
        if mode in avail_modes:
            if mode == "live":
                cmd = self._format_write_command(str(rcvr_target),"MDL")
            elif mode == "spectrum":
                cmd = self._format_write_command(str(rcvr_target),"MDS")
            elif mode == "menu":
                cmd = self._format_write_command(str(rcvr_target),"MDM")
            else:
                print("Errror. How tf did I get here?")


            self._write_serial(cmd)
        else:
            print("Error. usnsupported video mode")

    def show_osd(self,rcvr_target): #ODE shows/enables the osd

        #TODO check range of rcvr_target and support list of rcvr_target
        cmd = self._format_write_command(str(rcvr_target),"ODE")
        self._write_serial(cmd)

    def hide_osd(self,rcvr_target): #ODD hides/disables the osd

        #TODO check range of rcvr_target and support list of rcvr_target
        cmd = self._format_write_command(str(rcvr_target),"ODD")
        self._write_serial(cmd)

    def set_osd_at_predefined_position(self,rcvr_target,desired_position): #ODP 
        if 0 <= desired_position <= 7:
                cmd = self._format_write_command(str(rcvr_target),"ODP" + str(desired_position))
                self._write_serial(cmd)

    def set_osd_string(self,rcvr_target,osd_str,*Args,**kwargs): #ID => Sets OSD string
        osd_str_max_sz = 12
        osd_str = osd_str[:osd_str_max_sz]
        cmd = self._format_write_command(str(rcvr_target),"ID" + str(osd_str))
        self._write_serial(cmd)

    def reboot(self,rcvr_target,*args,**kwargs): #RBR => Reboot receiver. Issue twice to take effect.
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # cmd = self._format_write_command(str(rcvr_target),"RBR")
        # self._write_serial(cmd)
        # sleep(0.1)
        # self._write_serial(cmd)

    def reset_lock(self,rcvr_target,*args,**kwargs): #RL => Reset Lock 

        #TODO check range of rcvr_target and support list of rcvr_target
        cmd = self._format_write_command(str(rcvr_target),"RL")
        self._write_serial(cmd)
        self._write_serial(cmd)

    def set_video_format(self,rcvr_target,video_format,*args,**kwargs): #VF => Temporary Set video format. options are 'N' (ntsc),'P' (pal),'A' (auto)

        #TODO check range of rcvr_target and support list of rcvr_target
        desired_video_format = video_format.lower()
        if desired_video_format == "n" or desired_video_format == "ntsc":
            cmd = self._format_write_command(str(rcvr_target),"VFN")
            self._write_serial(cmd)
        elif desired_video_format == "p" or desired_video_format == "pal":
            cmd = self._format_write_command(str(rcvr_target),"VFP")
            self._write_serial(cmd)
        elif desired_video_format == "a" or desired_video_format == "auto":
            cmd = self._format_write_command(str(rcvr_target),"VFA")
            self._write_serial(cmd)
        else:
            print("Error. Invalid video format in set_temporary_video_format of ",desired_video_format)

    def move_cursor(self,rcvr_target,desired_move):
        # This command works but needs written out here
        raise NotImplementedError

        # TODO Use the _move_cursor() function

    def set_temporary_osd_string(self,rcvr_target,osd_str,*Args,**kwargs): #TID => Temporary set OSD string
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # cmd = self._format_write_command(str(rcvr_target),"TID" + str(osd_str))
        # self._write_serial(cmd)

    def set_temporary_video_format(self,rcvr_target,video_format,*args,**kwargs): #TVF => Temporary Set video format. options are 'N' (ntsc),'P' (pal),'A' (auto)
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError
        
        # desired_video_format = video_format.lower()
        # if desired_video_format == "n" or desired_video_format == "ntsc":
        #     cmd = self._format_write_command(str(rcvr_target),"TVFN")
        #     self._write_serial(cmd)
        # elif desired_video_format == "p" or desired_video_format == "pal":
        #     cmd = self._format_write_command(str(rcvr_target),"TVFP")
        #     self._write_serial(cmd)
        # elif desired_video_format == "a" or desired_video_format == "auto":
        #     cmd = self._format_write_command(str(rcvr_target),"TVFA")
        #     self._write_serial(cmd)
        # else:
        #     print("Error. Invalid video format in set_temporary_video_format of ",desired_video_format)

    def send_report_cstm(self, rcvr_target, *args, **kwargs): #RP XX => Custom report

        #TODO check range of rcvr_target and support list of rcvr_target
        while True:
            self._clear_serial_in_buffer()
            cmd = self._format_write_command(str(rcvr_target),input('Enter a command'))
            self._write_serial(cmd)
            sleep(0.05)
            report = self._read_until_termchar()
            print(report)
    

    ############################
    #       Broadcast Messages #
    ############################
    def set_all_receiver_addresses(self,new_address):
        self.set_receiver_address(0,new_address)

    def set_all_osd_message(self,message):
        self.set_osd_string(0,message)
        
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
    TODO the rx_id should match the sent rcvr_target
    
    Note: Reports are not intended to be requested of multiple receivers at the same time using broadcast. 
        The only situation this makes sense is if you don't know the address. 

        To find an unknown address, the following options exist
            1. use set_address(0,my_new_address). Ensure only one unit is connected otherwise they all get set to my_new_address
            2. use get_connected_receiver_list to see what the id is without changing it and update the known receiver address
            3. use get_address(0) to find the address. Ensure only one unit is connected    

    """

    def get_address(self, rcvr_target):
        """RPAD => Get the address of a unit if it's connected. Only useful to see what units are connected to the bus
        Returns namedtuple of receiver address with fields requestor_id, rx_id, and replied_address"""
        # TODO do I try to recatch serial exceptions here?



        report_name = "RPAD"
        pattern = r'\n([0-9])([0-9])AD([1-8])!\r' #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('address_report', ['requestor_id', 'rx_id', 'replied_address'])

        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

        
    def get_band_channel(self,rcvr_target):
        """RPBC => Get the band's channel number
        Returns namedtuple of receiver band channel with fields requestor_id, rx_id, and channel"""

        # TODO do I try to recatch serial exceptions here?
        report_name = "RPBC"
        pattern = r'\n([0-9])([0-9])BC([1-8])!\r' #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('band_channel_report', ['requestor_id', 'rx_id', 'channel'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_band_group(self,rcvr_target): 
        """RPBG => Get the band group
        Returns namedtuple of receiver band with fields requestor_id, rx_id, and band_index
        
        TODO convert from band index to an actual band name using the maps in clearview_comspecs.
        Change variable from band_index to band_name"""

        # TODO do I try to recatch serial exceptions here?
        report_name = "RPBG"
        pattern = r'\n([0-9])([0-9])BG([1-8,a-f])!\r' #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('band_group_report', ['requestor_id', 'rx_id', 'band_index'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_frequency(self,rcvr_target): 
        """RPFR => Get the frequency
        Returns namedtuple of receiver band channel with fields requestor_id, rx_id, and channel"""

        # TODO do I try to recatch serial exceptions here?
        report_name = "RPFR"
        n_digits_frequency = 4
        pattern =r'\n([0-9])([0-9])FR([0-9]{%s})!\r' % n_digits_frequency #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('frequency_report', ['requestor_id', 'rx_id', 'frequency'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_osd_string(self,rcvr_target): 
        """ RPID => Get the osd string, also called the "ID". Not to be confused with receiver address.
        Returns a namedtuple of receiver osd string with fields requestor_id, rx_id, and osd_string
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPID"
        pattern =r'\n([0-9])([0-9])ID(.{0,12})!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('osd_string_report', ['requestor_id', 'rx_id', 'osd_string'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_lock_format(self,rcvr_target): 
        """ RPLF => Get the lock format. Used to tell if receiver is locked to the video signal
        Returns a namedtuple of receiver lock format with fields requestor_id, rx_id, chosen_camera_type, forced_or_auto,locked_or_unlocked
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPLF"
        pattern =r'\n([0-9])([0-9])LF([NP])([FA])([LU])!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('osd_string_report', ['requestor_id', 'rx_id', 'chosen_camera_type','forced_or_auto','locked_or_unlocked'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_mode(self,rcvr_target): 
        """ RPMD => Get the mode : live, menu, or spectrum analyzer
        Returns a namedtuple of receiver mode with fields requestor_id, rx_id, mode
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPMD"
        pattern =r'\n([0-9])([0-9])MD([LMS])!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('osd_string_report', ['requestor_id', 'rx_id', 'mode'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_model_version(self,rcvr_target): 
        """ RPMV => Get the model version for hardware and software versions
        Returns a namedtuple of model_version with fields requestor_id, rx_id, hardware_version, and software_version_major, software_version_minor
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPMV"
        pattern =r'\n([0-9])([0-9])MV (CV[0-9].[0-9])-V([0-9]{2}.[0-9]{2})([A-Z]:[A-Z])!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('model_version_report', ['requestor_id', 'rx_id', 'hardware_version','software_version_major','software_version_minor'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_rssi(self,rcvr_target):
        """ RPRS => Get the rssi of each antenna
        Returns a namedtuple of rssi with fields requestor_id, rx_id, TODO,TODO,TODO,TODO
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPRS"
        pattern =r'\n([0-9])([0-9])RS([0-9]+),([0-9]+),([0-9]+),([0-9]+)!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('rssi_report', ['requestor_id', 'rx_id', 'TODO_a','TODO_b','TODO_c','TODO_d'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_osd_state(self,rcvr_target):
        """ RPOD => Get if OSD is enabled or disabled
        Returns a namedtuple of osd_state with fields requestor_id, rx_id, osd_state
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPOD"
        pattern =r'\n([0-9])([0-9])OD([ED])!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('osd_state_report', ['requestor_id', 'rx_id', 'osd_state'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)

    def get_video_format(self,rcvr_target):
        """ RPVF => Get Video Format (pal,ntsc,auto)
            Returns a namedtuple of video_format with fields requestor_id, rx_id, video_format
        """
        # TODO do I try to recatch serial exceptions here?
        report_name = "RPVF"

        pattern =r'\n([0-9])([0-9])VF([NAP])!\r'  #TODO replace hardcoded values with variables
        reply_named_tuple = namedtuple('video_format_report', ['requestor_id', 'rx_id', 'video_format'])
        return self._run_report(rcvr_target, report_name, pattern, reply_named_tuple)


    def get_report_cstm(self, rcvr_target, *args, **kwargs): #RP XX => Custom report
        while True:
            self._clear_serial_in_buffer()
            cmd = self._format_write_command(str(rcvr_target),"RP" + input('Enter a report type:'))
            self._write_serial(cmd)
            sleep(0.05)
            report = self._read_until_termchar()

            return report
            # if report == False:
            #     print("Error: No Report Reply. Either no receiver exists here or the report request is invalid")
            # elif report[-2:] != '!\r':
            #     print("Error: Invalid packet csum")
            # elif report.find("FR") != -1:
            #     report = report[report.find("RP"):]
            #     print("Reply:",report)

    #Use this to see which device ID's are connected
    def get_connected_receiver_list(self):
        connected_receivers = {}
        for i in range(1,9):
            self._clear_serial_in_buffer()

            if self.get_address(i) is not None:
                connected_receivers[i] = True
            else:
                connected_receivers[i] = False
            sleep(0.1)
        return connected_receivers

    def _run_report(self, rcvr_target, report_name, pattern, reply_named_tuple):
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

        self._clear_serial_in_buffer()
        cmd = self._format_write_command(str(rcvr_target),report_name)
        self._write_serial(cmd)
        sleep(0.05)

        if (report := self._read_until_termchar()) is not None:
            
            # Try matching based on expected behavior
            match = re.search(pattern,report)
            report = None
            if match:
                report = reply_named_tuple._make(match.groups())
            else:
                self.logger.error("When reporting %s from unit id#%s, the report of %s was not parsable.", report_name , rcvr_target, report.strip())
        else:
            self.logger.error("When reporting %s from unit id#%s, there was no response", report_name , rcvr_target)
        sleep(0.05)
        return report

    # ########################### 
    # ### CV Serial Utilities ### 
    # ###########################

    def _send_custom_command(self,rcvr_target,cstm_cmd):
        cmd = self._format_write_command(str(rcvr_target),cstm_cmd)
        self._write_serial(cmd)

    def _read_serial_blocking(self,line_ending_char): #waits for data to come in as long as it takes
        rv = ""
        line_ending_byte = line_ending_char.encode() #converting string char to byte. Example. '\n' converts to b'\n' , '>' converts to b'>'
        while True: #read all available bytes till the line_ending_byte
            ch = self._serial.read() 
            if ch == line_ending_byte:
                return rv #ignore the line ending byte
            rv += ch.decode() #push the bytes onto a string by decoding and appending

    def _format_write_command(self,rcvr_target,message): #formats sending commands with starting chars, addresses, csum, message and ending char. Arguments may be strings or ints
        return self.msg_start_char + str(rcvr_target) + str(self.mess_src) + str(message) + self.csum + self.msg_end_char

    def _move_cursor(self,rcvr_target,direction):
        cmd = self._format_write_command(str(rcvr_target),"WP"+direction)
        self._write_serial(cmd)
    
    def _print(self,*args,**kwargs):
        if self.debug_msg==True:
            print(args,sep=kwargs.get('sep','\t'))  

    def _write_serial(self,msg):
        self._print("CV_serial_write: ",sys._getframe().f_back.f_back.f_code.co_name , "=>" ,sys._getframe().f_back.f_code.co_name,msg,"cmd:",msg,**{'sep':' '})
        self._serial.write(msg.encode())

    def _get_serial_in_waiting(self):
        try:
            in_wait = self._serial.in_waiting
            return in_wait
        except OSError:
            raise
        
    def _clear_serial_in_buffer(self):
        try:
            while self._get_serial_in_waiting() > 0:
                self._serial.read()
            return True
        except OSError:
            raise

    def _read_until_termchar(self):
        str_read = self._serial.read_until(terminator = self.msg_end_char).decode('unicode_escape')
        if str_read == '':
            return None
        else:
            return str_read
        


if __name__ == "__main__": 
    print("This is ClearView module. Not main. Run main.py to understand how to use ClearView.py")

