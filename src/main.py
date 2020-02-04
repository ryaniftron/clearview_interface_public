import ClearView
import time

if __name__ == "__main__": #example usage
    rx_id = 1   #ensure the cv is set to that rx_id
    bc_id = 0
    cv = ClearView.ClearView(
        port= '/dev/ttyUSB0', #port name, COMX on windows
        debug = True,
        robust = True, #checks all data sent was actually received. Slows stuff down. TODO not implemented well yet 
        timeout = 0.1, #serial read timeout
        )
   
    ### Change receiver address from 4 to 1
    #cv.set_receiver_address(4,1)

    ### Set all receivers to address 1
    #cv.set_receiver_address(0,1)

    ### Get a list of connected receivers
    connected_receivers = cv.get_connected_receiver_list()
    print(connected_receivers)

    
    ### Antenna Mode
    # for i in range(4):
    #     print("Setting antenna mode to ",i)
    #     cv.set_antenna_mode(rx_id,i)
    #     time.sleep(3)


    #### band channel test
    # for i in range(8):
    #     print("setting band channel # to ",i+1)
    #     cv.set_band_channel(rx_id,i+1)
    #     time.sleep(3)


    ###### band test
    # bg_options = [0,1,2,3,4,5,6,7,8,9,'a','b','c','d','e','f']
    # for bg in bg_options:
    #     print("setting band to ",bg)
    #     cv.set_band_group(rx_id,bg)
    #     time.sleep(2)

    ###### OSD String Set
    #osd_string = "PILOT1"
    #cv.set_osd_string(rx_id, osd_string)


    #### Broadcast OSD Message
    #cv.set_all_osd_message("HELLO ALL")


    ###### Video Mode (show live,spectrum analyzer, menu)
    # mode_seq = ["live","menu","spectrum","live","spectrum","menu","live"]
    # for mode in mode_seq:
    #     print("Setting video display mode to ", mode)
    #     cv.set_video_mode(rx_id,mode)
    #     time.sleep(3)

    ##### Reset Lock
    # cv.reset_lock(rx_id)

    ##### Video Format
    # formats = ["n","p","a"]
    # for format_i in formats:
    #     print("vf:",format_i)
    #     cv.set_video_format(rx_id,format_i)
    #     time.sleep(3)

    ###### Enable and Disable OSD
    #cv.show_osd(rx_id)
    # time.sleep(3)
    # cv.hide_osd(rx_id)
    # time.sleep(3)
    # cv.show_osd(rx_id)
    # time.sleep(3)
    # cv.hide_osd(rx_id)
    
    #### Set osd position
    # for i in range(8):
    #     #0 to 7
    #     print("Setting osd to position",i)
    #     cv.set_osd_position(rx_id,i)
    #     time.sleep(5)


    print("Done")