rx_data = {
   "rx1": {'connection': '1', 'cam_forced_or_auto': 'F', 'lock_status': 'L', 'chosen_camera_type': 'N','node_number':'1'},
   "rx2": {'connection': '1', 'cam_forced_or_auto': 'F', 'lock_status': 'L', 'chosen_camera_type': 'N','node_number':'2'},
   "rx3": {'connection': '1', 'cam_forced_or_auto': 'F', 'lock_status': 'U', 'chosen_camera_type': 'N'}
}
if __name__ == "__main__":
    # Return a list of receivers that are both connected and locked

    def is_locked_and_connected(v):
        return v["connection"] == "1" and v.get("lock_status",None) == 'L'

    lock_rx_list = [k  for k,v in rx_data.items() if is_locked_and_connected(v)]
    
    lock_node_list = set(v.get("node_number",None) for k,v in rx_data.items() if k in lock_rx_list)
    print(lock_node_list)
    #lock_nodes = [k for k,v in rx_data.]
