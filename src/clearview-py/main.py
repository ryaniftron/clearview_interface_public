import time
import logging

import ClearView


# Create a custom logger
logger = logging.getLogger()    # Get root logger
logger.setLevel(logging.INFO)
# create file handler which logs even debug messages
fh = logging.FileHandler("cv_main_log.log")
fh.setLevel(logging.INFO)
# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
# create formatter and add it to the handlers
formatter_fh = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter_fh)
formatter_ch = logging.Formatter('logger.%(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter_ch)
# add the handlers to the logger
logger.addHandler(fh)
logger.addHandler(ch)


# If using the simulator, define the load and save file names. They usually are the same if you want persistent settings
#sim_file_load_name = "myfiletoload.txt"
sim_file_load_name = "cv_saved_settings.txt"
sim_file_save_name = "cv_saved_settings.txt"


if __name__ == "__main__":      # example usage
    rx_id = 1               # ensure the cv is set to that rx_id
    bc_id = 0
    cv = ClearView.ClearView(
        port='/dev/ttyUSB0',    # port name, COMX on windows
        debug=False,
        # Slows stuff down. TODO not implemented well yet
        robust=True,            # checks all data sent was actually received
        timeout=0.1,            # serial read timeout
        simulate_serial_port=False,
        sim_file_load_name=sim_file_load_name,
        sim_file_save_name=sim_file_save_name
        )

    # commands_to_test = set_receiver_address
    # print(cv.set_receiver_address(rcvr_target=0, new_target=2))

    # print(cv.get_connected_receiver_list())
    # cv.set_band_channel(robust=False,rcvr_target=rx_id,band_channel=3)
    # cv.set_band_channel(rcvr_target=rxtg,band_channel=7)

    # time.sleep(1)
    # cv.set_receiver_address(rcvr_target=6,new_target=4)

    # Change receiver address from 4 to 1
    # cv.set_receiver_address(4,1)

    # Set all receivers to address 1
    # cv.set_receiver_address(0,1)

    # Get a list of connected receivers and software versions
    # connected_receivers = cv.get_connected_receiver_list()
    # for rxaddr, conn_stat in connected_receivers.items():
    #     if conn_stat:
    #         logger.info("Receiver #%s is connected and running software version %s",
    #                     rxaddr,
    #                     cv.get_model_version(rcvr_target=rxaddr).software_version_major)

    # Antenna Mode
    # for i in range(4):
    #     logger.info("Setting antenna mode to ",i)
    #     cv.set_antenna_mode(rx_id,i)
    #     time.sleep(3)


    # band channel test
    # for i in range(8):
    #     logger.info("setting band channel # to ",i+1)
    #     cv.set_band_channel(rx_id,i+1)
    #     time.sleep(3)


    # band test
    # bg_options = [0,1,2,3,4,5,6,7,8,9,'a','b','c','d','e','f']
    # for bg in bg_options:
    #     logger.info("setting band to ",bg)
    #     cv.set_band_group(rx_id,bg)
    #     time.sleep(2)

    # OSD String Set
    # osd_string = "PILOT1"
    # cv.set_osd_string(rx_id, osd_string)


    # Broadcast OSD Message
    # cv.set_all_osd_message("HELLO ALL")

    # Video Mode (show live,spectrum analyzer, menu)
    # mode_seq = ["live", "menu", "spectrum", "live", "spectrum", "menu", "live"]
    # for mode in mode_seq:
    #     logger.info("Setting video display mode to %s", mode)
    #     cv.set_video_mode(rx_id, mode)
    #     time.sleep(3)

    # Reset Lock
    #cv.reset_lock(rx_id)

    # Video Format
    # formats = ["n","p","a","n","p","a"]
    # for format_i in formats:
    #     logger.info("vf:",format_i)
    #     cv.set_video_format(rx_id,format_i)
    #     time.sleep(3)

    # Enable and Disable OSD
    # cv.show_osd(rx_id)
    # time.sleep(3)
    # cv.hide_osd(rx_id)
    # time.sleep(3)
    # cv.show_osd(rx_id)
    # time.sleep(3)
    # cv.hide_osd(rx_id)      
    # Set osd position
    # for i in range(8):
    #     #0 to 7
    #     logger.info("Setting osd to position",i)
    #     cv.set_osd_position(rx_id,i)
    #     time.sleep(5)

    reports = [
        cv.get_address,
        cv.get_band_channel,
        cv.get_band_group,
        cv.get_frequency,
        cv.get_osd_string,
        cv.get_lock_format,
        cv.get_mode,
        cv.get_model_version,
        cv.get_rssi,
        cv.get_osd_state,
        cv.get_video_format,
    ]

    # Generating a list of reports

    # logger.info("Generating a list of reports. Press any key to continue after each report.")
    # input()
    # for report in reports:
    #     logger.info(report.__name__)
    #     logger.info(report(rcvr_target=rx_id))
    #     input()

    # while True:
    #     cv.send_report_cstm(rx_id)

    # ########### Example Setters ############
    # cv.set_address(robust=False, rcvr_target=rx_id, new_target=rx_id)
    # cv.set_antenna_mode(rcvr_target=rx_id, antenna_mode=1)
    # cv.set_band_channel(robust=False, rcvr_target=rx_id, band_channel=0)
    # cv.set_band_group(rcvr_target=rx_id, band_group=1)
    # cv.set_video_mode(rcvr_target=rx_id, mode="live")
    # cv.show_osd(rcvr_target=rx_id)
    # cv.hide_osd(rcvr_target=rx_id)
    # cv.set_osd_at_predefined_position(rcvr_target=rx_id, desired_position=0)
    # cv.set_osd_string(rcvr_target=rx_id, osd_str="TestPilot1")
    # cv.reset_lock(rcvr_target=rx_id)
    # cv.set_video_format(rcvr_target=rx_id, video_format="n")

    # ########### Example Getters #############
    print(cv.get_connected_receiver_list())
    print(cv.get_address(rcvr_target=rx_id))
    print(cv.get_band_channel(rcvr_target=rx_id))
    print(cv.get_band_group(rcvr_target=rx_id))
    print(cv.get_frequency(rcvr_target=rx_id)) #Not implemented. Need to make band chart table"""
    print(cv.get_osd_string(rcvr_target=rx_id))
    print(cv.get_lock_format(rcvr_target=rx_id)) #Not implemented. Need to set up the rf simulator"""
    print(cv.get_mode(rcvr_target=rx_id))
    print(cv.get_model_version(rcvr_target=rx_id)) #Not implemented."""
    print(cv.get_rssi(rcvr_target=rx_id)) #Not implemented"""
    print(cv.get_osd_state(rcvr_target=rx_id))
    print(cv.get_video_format(rcvr_target=rx_id))

    # cv.set_video_mode(rcvr_target=rx_id, mode="menu")
    # cv.set_osd_string_positional(rcvr_target=rx_id, starting_index=0, osd_str="1234")

    # cv._clear_serial_in_buffer()
    # cmd = cv._format_write_command(str(0), "RPVF")
    # cv._write_serial(cmd)
    # print(cv._read_until_termchar())

    #cv._write_serial(cv._format_write_command(rcvr_target=int(rx_id), message="AN0"))
    #cv.send_report_cstm(rcvr_target=rx_id,custom_command="RPID")

    # for i in range(8):
    #     cv._run_command(rcvr_target=rx_id, message="ID123456789ab"+str(i))
    #     time.sleep(0.2)

    # for i in range(6):
    #     print(cv._read_until_termchar())
    # print(cv.get_connected_receiver_list())

    # cv.hide_osd(rcvr_target=rx_id)
    # time.sleep(5)
    # cv.set_user_message(rcvr_target=rx_id, osd_str="A nice message")
    # time.sleep(2)
    # cv.set_user_message(rcvr_target=rx_id, osd_str="")
    # time.sleep(2)
    # cv.show_osd(rcvr_target=rx_id)
    # time.sleep(2)
    # cv.set_user_message(rcvr_target=rx_id, osd_str="Back again!")
    # time.sleep(2)
    # cv.set_user_message(rcvr_target=rx_id, osd_str="")
    cv.show_osd(rcvr_target=rx_id)
    time.sleep(0.2)
    cv.set_user_message(rcvr_target=rx_id, osd_str='Back again$%&()*+-_/:;<=>?@{|}~')
    time.sleep(0.2)
    print(cv.get_rssi(rcvr_target=rx_id))
    # cv.set_user_message(rcvr_target=rx_id,osd_str=" ")
    # count = 1
    # time.sleep(0.1)
    #cv.set_user_message(rcvr_target=rx_id, osd_str="")
    # while True:
    #     pass
    # while True:
    #     dt = 0.5
    #     time.sleep(dt)
    #     cv.set_user_message(rcvr_target=rx_id, osd_str="RotorHazard")

    #     time.sleep(dt)
    #     cv.set_user_message(rcvr_target=rx_id, osd_str="A really long 505050 character message is here now")

    #     time.sleep(dt)
    #     cv.set_user_message(rcvr_target=rx_id, osd_str="X")

    #     time.sleep(dt)
    #     cv.set_user_message(rcvr_target=rx_id, osd_str="")
        
    #     if count % 5 == 0:
    #         print("Left")
    #         time.sleep(dt)
    #         cv.set_osd_at_predefined_position(rcvr_target=rx_id,desired_position=1)
    #         time.sleep(3)
    #     elif count % 5 == 3:
    #         print("Right")
    #         time.sleep(dt)
    #         cv.set_osd_at_predefined_position(rcvr_target=rx_id,desired_position=5)
    #         time.sleep(3)


    #     count = count + 1

    

    # dt = 0.1
    # while True:
    #     time.sleep(dt)
    #     cv.set_user_message(rcvr_target=rx_id, osd_str="RH" + str(count%5))
    #     count = count+1


print("\n\n")
logging.info("Shutting down logger and closing out")
logging.shutdown()
