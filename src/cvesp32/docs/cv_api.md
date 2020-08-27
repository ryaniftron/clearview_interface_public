# CVCM API

The CVCM supports JSON over http and also over MQTT. The underlying API is the same. For http, the data can be sent as either JSON or raw key/value pairs from a form, but data is always returned as JSON. 


## API Endpoints

Some settings support reading the parameter. To read a parameter, simply pass in the '?' character. 
| API Endpoint     | Description                                                            | Read/Write | Example Reply                     | Possible Values                            |
|------------------|------------------------------------------------------------------------|------------|-----------------------------------|--------------------------------------------|
| ssid             | SSID the CVCM will connect to                                          | R/W        | mywifinetwork                     | <=32 character string                      |
| password         | Password the CVCM will connect to                                      | R/W        | mypassword                        | <=32 character string                      |
| device_name      | Device name visible by others                                          | R/W        | ryan's_cv                         | <=32 character string                      |
| broker_ip        | MQTT Broker IP Address                                                 | R/W        | 192.168.0.25                      | <=32 character string                      |
| seat_number      | Seat Number that it follows for frequencies                            | R/W        | 3                                 | 0 <= seat_number <= 7                      |
| antenna_mode     | Antenna Mode                                                           | W          | ---                               |                                            |
| channel          | Video Channel                                                          | R/W        | 3                                 |                                            |
| band             | Video Band                                                             | R/W        | a                                 |                                            |
| id               | Set osd "ID" string, traditionally used for pilot handle               | R/W        | pilot1                            |                                            |
| user_message     | Set longer user message                                                | R/W        | Lap:4 +0.56                       |                                            |
| mode             | Change what is displayed between menu, spectrum analyzer or live video | W          | ---                               |                                            |
| osd_visibility   | Show or Hide OSD                                                       | W          | ---                               |                                            |
| osd_position     | Change OSD Position                                                    | W          | ---                               |                                            |
| lock             | Resets the CV Lock or queries lock status                              | R/W        | ---                               | Reset lock with '1', or get value with '?' |
| video_format     | Camera type                                                            | W          | ---                               |                                            |
| cv_version       | Firmware Version                                                       | R          | 1.21a                             | Get the firmware version of the CV         |
| cvcm_version     | CVCM Version                                                           | R          | v1.21.a3                          | Get the short firmware version of the CVCM |
| cvcm_version_all | CVCM Version with build date                                           | R          | v1.21.a3 - Aug 26 2020 - 23:44:28 | Get the long firmware version of the CVCM  |

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

## Return Codes:

If the API request is formatted or other error occurs, the following may be returned 

* `error-no_comms` - it means the cv is not responding to the UART requests
Perhaps there is a firmware version mismatch or cabling issue
* `error-invalid_endpoint` - the api endpoint requested does not exist
* `error-write` - this parameter is intended to be read-only
* `error-read` - this parameter is intended to be write-only
* `error-value` - the value specified to change is out of range

For example, if a post to read the a non-existant parameter called `n_bugs` was called, the return would be this:
* `{"n_bugs":"error-invalid_endpoint"}`

If you tried to set the band to an invalid band `v` with `{"band":"v"}`, the following would return
* `{"band":"error-value"}`




