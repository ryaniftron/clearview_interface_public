# ClearView ESP32 Code

This README explains the necessary functionality of the ClearView ESP32 Dongle. The ESP32's purpose is to allow WiFi connectivity of a ClearView. In it's simplest form, the ESP32 must allow a user to configure the network credentials the dongle will try to connect to and then listen for commands once it joins the wifi network.

## SoftAP

When the ESP32 first boots up after flashing, it will generate its own hotspot. The SSID will be the serial number of the ESP32. It will have one of those sign-in popups which will direct you to a web page it generates. The web page will have the following options:
 
 * ssid (text entry)
 * password (text entry)
 * device name (text entry)
 * connect to network (button)

 Thus, when the ESP32 dongle is going to join an existing network to listen for commands, the network credentials can be entered using the Web UI. When the "connect to network" button is pressed, the ESP32 will turn off SoftAP and try connecting. If it fails, it will restart the SoftAP until told to try connecting again. 


## Client Mode

If the correct credentials are entered and the ESP32 can join the wifi network, it then will be a client. The ESP32 can listen to commands and report requests. MQTT will be used to accomplish this for a number of reasons. 



# WiFi softAP example

(See the README.md file in the upper level 'examples' directory for more information about examples.)


## How to use example

### Configure the project

```
idf.py menuconfig
```

* Set serial port under Serial Flasher Options.

* Set WiFi SSID and WiFi Password and Maximal STA connections under Example Configuration Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

There is the console output for this example:

```
I (917) phy: phy_version: 3960, 5211945, Jul 18 2018, 10:40:07, 0, 0
I (917) wifi: mode : softAP (30:ae:a4:80:45:69)
I (917) wifi softAP: wifi_init_softap finished.SSID:myssid password:mypassword
I (26457) wifi: n:1 0, o:1 0, ap:1 1, sta:255 255, prof:1
I (26457) wifi: station: 70:ef:00:43:96:67 join, AID=1, bg, 20
I (26467) wifi softAP: station:70:ef:00:43:96:67 join, AID=1
I (27657) tcpip_adapter: softAP assign IP to station,IP is: 192.168.4.2
```
