# CVCM API

The CVCM supports JSON over http and also over MQTT. The underlying API is the same. For http, the data can be sent as either JSON or raw key/value pairs from a form, but data is always returned as JSON. 


## API Endpoints

Some settings support reading the parameter. To read a parameter, simply pass in the '?' character. 
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
| user_msg     | Set longer user message                                                | R/W        | Lap:4 +0.56                       |                                            |
| mode             | Change what is displayed between menu, spectrum analyzer or live video | R/W          | ---                               |                                            |
| osd_visibility   | Show or Hide OSD                                                       | R/W          | ---                               |                                            |
| osd_position     | Change OSD Position                                                    | W          | ---                               |                                            |
| lock             | Resets the CV Lock or queries lock status                              | R/W        | ---                               | Reset lock with '1', or get value with '?' |
| video_format     | Camera type                                                            | R/W          | ---                               |                                            |
| cv_version       | Firmware Version                                                       | R          | 1.21a                             | Get the firmware version of the CV         |
| cvcm_version     | CVCM Version                                                           | R          | v1.21.a3                          | Get the short firmware version of the CVCM |
| cvcm_version_all | CVCM Version with build date                                           | R          | v1.21.a3 - Aug 26 2020 - 23:44:28 | Get the long firmware version of the CVCM  |
| led | Set LED State ( active until next state change)                                     | R?W          | on | on,off,blink_slow,blink_fast,breathe_slow,breathe_fast  | 
| send_cmd | Send arbitrary UART command | W | --- |
| req_report | Send arbitrary UART report | W | --- |
| mac_addr | Get mac address of wifi | R | CV_342832C40A24 | 
| ip_addr | Get ip address of wifi | R | 192.168.4.1 |
| wifi | Wifi state | R/W | ['ap','sta'] | ['ap','sta']

## MQTT Topics

TODO

## HTTP Endpoints

* URL
    * `/settings`
* Method: 
    * `POST`
* Headers Needed
    * `Content-Type` = `application/json` for JSON 
    * `Content-Type` = `text/html` for plaintext



## Examples

### HTTP

To get the cvcm_version using JSON, send a POST to `192.168.4.1/settings` with the content-type header of JSON
    * content: `{"cvcm_version", "?"}
    * reply: `{"cvcm_version", "v1.21.a2"}
The request could also have been done with content-type header of text/html
    * content: `cvcm_version=?`
    * reply: `{"cvcm_version", "v1.21.a2"}
    Note how the reply is always JSON

When values are set, the JSON is returned. If there is an error, the key "error" is returned with some debug info. 

Note: It would be fantastic to only use JSON, but HTML forms don't normally submit as JSON. 
There is code [here](https://github.com/keithhackbarth/submitAsJSON) that may be a good replacement.

## Return Codes:

If the API request is formatted or other error occurs, the following may be returned 

* `error-no_comms` - it means the cv is not responding to the UART requests
Perhaps there is a firmware version mismatch or cabling issue
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

There is a desire to have combined reports that when requesting a combined report, multiple values are returned back. Information would be added here.

