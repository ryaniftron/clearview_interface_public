# CVCM API

The CVCM supports JSON over http and also over MQTT. The underlying API is the same. For http, the data can be sent as either JSON or raw key/value pairs from a form, but data is always returned as JSON.


## API Endpoints

Some settings support reading the parameter. To read a parameter, simply pass in the '?' character.
For a reference to what some of the raw values that the ClearView2.0 will accept, see [here](https://docs.google.com/document/d/1-1uMS89gDI37topBPb-QPKKed2rIyEi6Y7M0LAVGZGE/edit?usp=sharing)
| API Endpoint Key | Description                                                            | Read/Write | Example Reply Value               | Possible Values                            |
|------------------|------------------------------------------------------------------------|------------|-----------------------------------|--------------------------------------------|
| address          | Address the CVCM UART is at (unused)                                   | R/W        | 1                                 | 0 <= address <= 7 |
| ssid             | SSID the CVCM will connect to                                          | R/W        | mywifinetwork                     | <=32 character string                      |
| password         | Password the CVCM will connect to                                      | R/W        | mypassword                        | <=32 character string                      |
| device_name      | Device name visible by others                                          | R/W        | ryan's_cv                         | <=32 character string                      |
| broker_ip        | MQTT Broker IP Address                                                 | R/W        | 192.168.0.25                      | <=32 character string                      |
| seat             | Seat Number that CVCM follows for frequencies                          | R/W        | 3                                 | 0 <= seat <= 7                      |
| antenna     | Antenna Mode                                                           | W          | ---                               |                                            |
| channel          | Video Channel                                                          | R/W        | 3                                 |                                            |
| band             | Video Band                                                             | R/W        | a                                 |                                            |
| id               | Set osd "ID" string, traditionally used for pilot handle               | R/W        | pilot1                            |                                            |
| user_msg     | Set longer user message, or blank to disable                  | R/W        | Lap:4 +0.56                       |                                            |
| mode             | Change what is displayed between menu, spectrum analyzer or live video | R/W          | ---                               |                       'L','M','S' for Live,Menu, and Spectrum Analyzer                     |
| osd_visibility   | Show or Hide OSD                                                       | R/W          | ---                               |                                            |
| osd_position     | Change OSD Position                                                    | W          | ---                               |                                            |
| lock             | Gets lock status                              | R       | ---                               | TODO |
| reset_lock             | Resets the CV Lock (NOT IMPLEMENTED)                              | W        | ---                               | Reset lock with '1'. Will confirm back with 1 if successful |
| video_format     | Camera type                                                            | R/W          | ---                               |                                            |
| cv_version       | Firmware Version                                                       | R          | 1.21a                             | Get the firmware version of the CV         |
| cvcm_version     | CVCM Version                                                           | R          | v1.21.a3                          | Get the short firmware version of the CVCM |
| cvcm_version_all | CVCM Version with build date                                           | R          | v1.21.a3 - Aug 26 2020 - 23:44:28 | Get the long firmware version of the CVCM  |
| led | Set LED State ( active until next state change)                                     | R?W          | on | on,off,blink_slow,blink_fast,breathe_slow,breathe_fast  |
| send_cmd | Send arbitrary UART command | W | --- |
| req_report | Send arbitrary UART report | W | --- |
| mac_addr | Get mac address of wifi | R | CV_342832C40A24 |
| ip_addr | Get ip address of wifi | R | 192.168.4.1 |
| wifi_state | WiFi State | R/W | ['ap','sta'] | ['ap','sta']
| wifi_power | WiFi Power | R/W | [8,20,28,34,44,52,56,60,66,72,78]] multiply by 0.25 to get power in dBm
| ~~status_static~~ (not implemented) | Static Status | R | (dev, ver, cvcm_version, mac_addr, ip_addr) | |
| ~~status_variable~~ (not implemented) | Variable Status | R | (seat_number) | |
| device_type | Device Type | R | ['rx'] | Get the device type. Video receivers reply with rx |


## HTTP Endpoints

* URL
    * `/settings`
* Method:
    * `POST`
* Headers Needed
    * `Content-Type` = `application/json` for JSON
    * ~~`Content-Type` = `text/html` for plaintext~~ (DEPRECATED)

## MQTT Topics

* Commands
  * JSON for all units: `rx/cv1/cmd_esp_all`
  * JSON for all units at a seat `0`: `rx/cv1/cmd_esp_seat/0`
  * JSON to single receiver `CV_240AC4D3320C`: `rx/cv1/cmd_esp_target/CV_240AC4D3320C`

* Responses
  * JSON for any response: `rx/cv1/resp_target/CV_240AC4D3320C`


## Examples

### HTTP

To get the cvcm_version using JSON, send a POST to `192.168.4.1/settings` with the content-type header of JSON

* content: `{"cvcm_version", "?"}
* reply: `{"cvcm_version", "v1.21.a2"}

~~The request could also have been done with content-type header of text/html~~

* ~~content: `cvcm_version=?`~~
* ~~reply: `{"cvcm_version", "v1.21.a2"}~~
* ~~Note how the reply is always JSON~~
* This text/html method is now deprecated as of v1.21.a6

When values are set, the JSON is returned. If there is an error, the key "error" is returned with some debug info.

Note: It would be fantastic to only use JSON, but HTML forms don't normally submit as JSON.
There is code [here](https://github.com/keithhackbarth/submitAsJSON) that may be a good replacement.

### MQTT

First, configure the module to join a WiFi network and MQTT broker in the WiFi tab.
Next, make it enter station mode.
You should see it subscribed in the MQTT broker and publish a connection message

To get the ip address of all receivers using JSON, send a MQTT publish
* command topic: `rx/cv1/cmd_esp_all`
* command payload `{"ip_addr": "?"}`
* response topic: `rx/cv1/resp_target/CV_240AC4D3320C`
* response payload: `{"ip_addr":      "10.59.11.47"}`

To set the band of all receivers on seat 0 to band:2 and channel:1:
* command topic: `rx/cv1/cmd_esp_seat/0`
* command payload `{"band": "2", "channel": "1"}`
* response topic `rx/cv1/resp_target/CV_240AC4D3320C`
* response payload `{"band": "2", "channel": "1"}`


## Return Codes:

If the API request is formatted or other error occurs, the following may be returned

* `error-no_comms` - it means the cv is not responding to the UART requests
   * UI message: `Error: No response from the ClearView Video Receiver`
   * Perhaps there is a firmware version mismatch or cabling issue
* `error-no_wifi` - it means that the esp32 is no longer connected over wifi to the phone
   * UI message: `Error: No WiFi connection to dongle`
* `error-invalid_endpoint` - the api endpoint requested does not exist
* `error-write` - this parameter is intended to be read-only
* `error-read` - this parameter is intended to be write-only
* `error-value` - the value specified to change is out of range
* `error-nvs_read` - the value could not be read from flash
* `error-nvs_write` - the value could not be written flash

For example, if a post to read the a non-existant parameter called `n_bugs` was called, the return would be this:
* `{"n_bugs":"error-invalid_endpoint"}`

If you tried to set the band to an invalid band `v` with `{"band":"v"}`, the following would return
* `{"band":"error-value"}`

If the JSON request is malformatted, a general json error will return
* `{"error":"error-json-parse"}`
If the POST is not the right content type,


## Combined Requests

There is a desire to have combined reports that when requesting a combined report, multiple values are returned back. Information would be added here. For example, status_static, status_variable, and lock format all could return multiple keys of information with one key request. This would save data and also multiple serial reads for lock status.

