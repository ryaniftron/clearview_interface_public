This document serves as a user manual for the ESP32 Wireless Dongle.

# Update Instructions
## Check Firmware Version
## Push Update

# LED Codes
* Slow Blinking = Hotspot Mode, Errors exist and can be viewed
* Fast Blinking = Hotspot Mode, No errors
* Twinkling = Hotspot Mode, Connected Device
* Bright Solid = Attempting Connection to Network
* Fast Breathing = Connected to Network, Waiting for MQTT
* Slow Breathing = Connected to Network and MQTT is Active

# Usage Guide
After updating firmware of CV2.0 and pushing the latest OTA file to the dongle, follow the processes below

## Viewing Settings in Hotspot Mode

1. Connect the dongle to the ClearView and power on the ClearView
1. If this is the first time, the ESP32 will "Slow Blink"
1. In Hotspot Mode, the ESP32 generates a WiFi Hotspot. 
   * The SSID begins with "CV_". For example "CV_342932C40A32"
   * The network is unsecured (no password) and allows up to four devices to join
1. Using any laptop or phone with WiFi, go to the WiFi settings and join the network
1. Ignore any errors about not being connected to the internet. It's a local network.
1. Open a web browser and go to this web site [http://192.168.4.1](http://192.168.4.1)
1. It will give you the home page
1. **TODO** ---further--usage--here

# Troubleshooting
* Can't connect to WiFi
  * ESP32 only connects to 2.4GHz networks
  * Credentials are incorrect
  * Recommended to log back into Hotspot when it fails back to that and view the error message
