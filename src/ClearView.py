### ClearView.py
### Author: Ryan Friedman
### Date Created: Feb 3,2020
### License: GPLv3

### Change Log


# 2020-02-03
# Command Protocol 1.20
# https://docs.google.com/document/d/1oVvlSCK49g7bpSY696yi9Eg0pA1uoW88IxbgielnqIs/edit?usp=sharing

# Initial Release of v1.20.0 for CV1.20



#ClearView class for serial comms
import serial
from time import sleep
import inspect,sys #for printing caller in debug
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
    
    


class ClearView:
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
        
    # ###########################   
    # ### CV Control Commands ###
    # ########################### 
    def set_receiver_address(self,rcvr_targets,new_target,*args,**kwargs): #ADS => Sets receiver address as new_target
        """set_receiver_address => rcvr_target is the receiver seat number of interest,0-8, 0 for broadcast. new_target is the new seat 1-8"""
        try: #rcvr_targets is a list of sorts
            for rx in rcvr_targets:
                cmd = self._format_write_command(rx,"ADS" + str(new_target))
                self._write_serial(cmd)
        except: #rcvr_targets is an int or string
            cmd = self._format_write_command(int(rcvr_targets),"ADS" + str(new_target))
            self._write_serial(cmd)

    def set_antenna_mode(self,rcvr_target,antenna_mode): #AN
        if 0 <= antenna_mode <= 3:
            cmd = self._format_write_command(str(rcvr_target),"AN" + str(antenna_mode))
            self._write_serial(cmd)
        else:
            print("Error. Unsupported antenna mode of ", antenna_mode)


    def set_band_channel(self,rcvr_target,band_channel,*args,**kwargs): #BC = > Sets band channel. 1-8
        if 1 <= band_channel <= 8:
            cmd = self._format_write_command(str(rcvr_target),"BC" + str(band_channel - 1))
            self._write_serial(cmd)
        else:
            print("Error. set_band_channel target of ",band_channel, "is out of range")

    def set_band_group(self,rcvr_target,band_group,*args,**kwargs): #BG = > Sets band. 0-9,a-f
        #TODO add check if 0-9 and a-f
            cmd = self._format_write_command(str(rcvr_target),"BG" + str(band_group))
            self._write_serial(cmd)

    #broken 
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


    #good
    def set_video_mode(self,rcvr_target,mode): #MDL,MDS,MDM
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

    # good
    def show_osd(self,rcvr_target): #displays the osd
        cmd = self._format_write_command(str(rcvr_target),"ODE")
        self._write_serial(cmd)

    # good
    def hide_osd(self,rcvr_target): #hides the osd
        cmd = self._format_write_command(str(rcvr_target),"ODD")
        self._write_serial(cmd)

    # good
    def set_osd_at_predefined_position(self,rcvr_target,desired_position): #ODP
        if 0 <= desired_position <= 7:
                cmd = self._format_write_command(str(rcvr_target),"ODP" + str(desired_position))
                self._write_serial(cmd)

    #good
    def set_osd_string(self,rcvr_target,osd_str,*Args,**kwargs): #ID => Sets OSD string
        osd_str_max_sz = 12
        osd_str = osd_str[:osd_str_max_sz]
        cmd = self._format_write_command(str(rcvr_target),"ID" + str(osd_str))
        self._write_serial(cmd)



    #broken
    def reboot(self,rcvr_target,*args,**kwargs): #RBR => Reboot receiver. Issue twice to take effect.
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # cmd = self._format_write_command(str(rcvr_target),"RBR")
        # self._write_serial(cmd)
        # sleep(0.1)
        # self._write_serial(cmd)

    #good
    def reset_lock(self,rcvr_target,*args,**kwargs): #RL => Reset Lock 
        cmd = self._format_write_command(str(rcvr_target),"RL")
        self._write_serial(cmd)
        self._write_serial(cmd)

    #good
    def set_video_format(self,rcvr_target,video_format,*args,**kwargs): #VF => Temporary Set video format. options are 'N' (ntsc),'P' (pal),'A' (auto)
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


    #broken
    def set_temporary_osd_string(self,rcvr_target,osd_str,*Args,**kwargs): #TID => Temporary set OSD string
        # This command does not yet work in ClearView v1.20
        raise NotImplementedError

        # cmd = self._format_write_command(str(rcvr_target),"TID" + str(osd_str))
        # self._write_serial(cmd)

    #broken
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
    def get_frequency(self,rcvr_target,*args,**kwargs): #RP AD => Report Receiver Address
        #note: can use this to see if units are connected by asking for response

        # TODO
        raise NotImplementedError

        # self._clear_serial_in_buffer()
        # cmd = self._format_write_command(str(rcvr_target),"RPFR")
        # self._write_serial(cmd)
        # sleep(0.05)
        # fr_report = self._read_until_termchar()
        # if fr_report[-2:] != '!\r':
        #     print("Error: No valid requency report ending")
        #     return
        # elif fr_report.find("FR") != -1:
        #     fr_report = fr_report[fr_report.find("FR"):]
        #     frequency = int(fr_report[2:-2])
        #     return frequency
        # else:
        #     print("Error: Not a valid frequency report")

    def get_lock_status(self,rcvr_target,*args,**kwargs): #RP LF=> Report Lock Status
        # TODO
        raise NotImplementedError

        # self._clear_serial_in_buffer()
        # cmd = self._format_write_command(str(rcvr_target),"RPLF")
        # self._write_serial(cmd)
        # sleep(0.05)
        # fr_report = self._read_until_termchar()
        # if fr_report[-2:] != '!\r':
        #     print("Error: No valid lock status report ending")
        #     return
        # elif fr_report.find("LF") != -1:
        #     fr_report = fr_report[fr_report.find("LF"):]
        #     lock_status = fr_report[1:-2]
        #     return lock_status
        # else:
        #     print("Error: Not a valid lock status report")



    #good
    def get_model_version(self,rcvr_target,*args,**kwargs): #RP MV => Report model version
        self._clear_serial_in_buffer()
        cmd = self._format_write_command(str(rcvr_target),"RPMV")
        self._write_serial(cmd)
        sleep(0.05)
        model_report = self._read_until_termchar()
        if model_report == False:
            self._print("Error: No response from cv#",rcvr_target," on model report request")
            return False
        elif model_report[-2:] != '!\r':
            self._print("Error: No valid model report ending")
            return False

        elif model_report.find("MV") != -1:
            print("model report:",model_report)
            model_report = model_report[model_report.find("MV"):]
            model_str = model_report[2:-2]
            version_desc = model_str.strip(' ').split('-')
            model_dict = {
                'hw' : version_desc[0],
                'sw' : version_desc[1],
            }
            return model_dict

        else:
            self._print("Error: Invalid frequency report")
            return False

    
    #Use this to see which device ID's are connected
    def get_connected_receiver_list(self):
        connected_receivers = {}
        for i in range(1,9):
            self._clear_serial_in_buffer()

            if self.get_model_version(i) == False:
                connected_receivers[i] = False
            else:
                connected_receivers[i] = True
            sleep(0.1)
        return connected_receivers

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
        return self._serial.in_waiting

    def _clear_serial_in_buffer(self):
        while self._get_serial_in_waiting() > 0:
            self._serial.read()

    def _read_until_termchar(self):
        str_read = self._serial.read_until(terminator = self.msg_end_char).decode('unicode_escape')
        print("str_read:",str_read)
        if str_read == '':
            return False
        else:
            return str_read
        


if __name__ == "__main__": #example usage
    print("This is a module. Not main. Run main.py")

