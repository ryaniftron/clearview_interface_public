import logging

import clearview
from clearview.comspecs import clearview_specs, cv_device_limits, band_map_cv_to_display, band_map_display_to_cv

from time import sleep

logger = logging.getLogger(__name__)


# This module is for testing the CV2.0 protocol for both automated and manual tests

def test_manual(cv):

    #TODO in the raw input, you could generate a report based on pass or fail

    rx_id = clearview_specs["bc_id"]

    # # Antenna Mode
    # ams = ['DIVERSITY', 'LEFT', 'RIGHT', 'CLEARVIEW']
    # for i, am in enumerate(ams):
    #     cv.set_antenna_mode(rx_id, i)
    #     raw_input("Check Antenna Mode: %s:%s"%(i, am)) # Python2 only

    # # Band Channel
    # for bc in range(cv_device_limits["min_channel"], cv_device_limits["max_channel"] + 1):
    #     cv.set_band_channel(rx_id, bc)
    #     raw_input("Check Channel: %s"%(bc))

    # # band groups
    # for bg, bname in band_map_cv_to_display.items():
    #     cv.set_band_group(rx_id, bg)
    #     raw_input("Check Band: %s=>%s"%(bg,bname))

    # Set CV to R8 for further tests (ALWAYS)
    cv.set_band_channel(rx_id, 7)
    cv.set_band_group(rx_id, band_map_display_to_cv['R']) 



    # # User ID String
    # for osd_str in ["UPPERCASE", "lowercase", "3P$ia(", "MyReallyLongOSDString",""]:
    #     cv.set_osd_string(rx_id, osd_str)
    #     if osd_str == '':
    #         print("Turning off ID")
    #     raw_input("Check OSD String is '%s'"%(osd_str))


    # # Mode for showing live, menu, or spectrum
    # mode_seq = ["live", "menu", "spectrum", "live", "spectrum", "menu", "live"]
    # for mode in mode_seq:
    #     cv.set_video_mode(rx_id, mode)
    #     raw_input("Check video display mode to %s"% mode)


    # # OSD enable disable
    # cv.show_osd(rx_id)
    # raw_input("Check CV OSD visible")


    # cv.hide_osd(rx_id)
    # raw_input("Check CV OSD hidden")


    # cv.show_osd(rx_id)
    # raw_input("Check CV OSD visible")

    # # OSD Position
    # for i in range(cv_device_limits["min_osd_position"], cv_device_limits["max_osd_position"] + 1):
    #     cv.set_osd_at_predefined_position(rx_id,i)
    #     raw_input("Check osd in position %s"%i)
    # Move back to reasonable position
    cv.set_osd_at_predefined_position(rx_id,1)

    # for i in range(2): # Test a few times
    #     cv.reset_lock(rx_id)
    #     raw_input("Check lock was reset")

    # Show the menu now to check the camera type changes
    cv.set_video_mode(rx_id, "menu")

    # for vf in cv_device_limits["video_format_list"]:
    #     cv.set_video_format(rx_id, vf)
    #     raw_input("Check video format is %s"%vf)
    
    cv.set_video_format(rx_id,"N")

    cv.move_cursor(rx_id,'down')
    raw_input("Check moved down")

    cv.move_cursor(rx_id,'up')
    raw_input("Check moved up")

    cv.move_cursor(rx_id,'up')
    raw_input("Check moved up again")

    cv.move_cursor(rx_id, 'enter')













    
    


def test_automated(cv):
    bc_id = clearview_specs["bc_id"]

    # Set the receiver to addr 5
    desired_addr = 5
    cv.set_address(bc_id,desired_addr)
    sleep(0.2)
    try:
        new_addr = cv.get_address(bc_id).address
        assert int(desired_addr) == int(new_addr), "Set addr error: %s != %s"%(desired_addr, new_addr)
    except AssertionError:
        pass


    # Tell a different receiver to be addr 6 and check it's still 5
    ignore_addr = 6
    cv.set_address(3,ignore_addr)
    sleep(0.2)
    try:
        new_addr = cv.get_address(bc_id).address
        assert int(5) == int(new_addr), "Set addr error: %s != %s. Receiver shouldn't change"%(desired_addr, new_addr)
    except AssertionError:
        pass





    # cv.set_antenna_mode(bc_id, 5)
    # cv.hide_osd(bc_id)
    # sleep(1)
    # cv.set_all_osd_message("Hello World")
    # sleep(1)
    # cv.show_osd(bc_id)
    # sleep(1)
    # cv.set_all_osd_message(" ")



if __name__ == "__main__":    
    logging.basicConfig()

    cv = clearview.ClearView(
        port="/dev/ttyUSB0",    # port name, COMX on windows
        debug=False,
        # Slows stuff down. TODO not implemented well yet
        robust=False,            # checks all data sent was actually received
        timeout=0.1,            # serial read timeout
        simulate_serial_port=False,
        return_formatted_commands=False,
    )

    # test_automated(cv)
    test_manual(cv)

    ()
