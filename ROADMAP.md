# Roadmap

This details the expected features and breaking compatabilites. 

## CVCM 1.21.a4

* Minimum CV2.0 FW: 1.21a
* Support reading values from the CV2.0
* Support UM command
* Add testing UI improvements and custom commands/reports
* Change all code to reference "seat" including MQTT
* a4 has a new JSON API, but only has endpoints on the web server.
  *  MQTT access will be released later
  * The web server uses the JSON API only
  * The web server has javascript embedded UI

### RH changes

* RH change to "seat" reference instead of "node" in the MQTT topics
* RH change to "seat_number" in MQTT status

## CVCM 1.24.a1

* a1 supports automatic replies from any commands
  * These are given back to the UI, but no check takes place to ensure the value sent is the value received
* a1 supports new commands for update flags, user message report, and improved frequency setting

## Future features

* CVCM supports  it's JSON API over MQTT
* RH use JSON API instead of formatting commands and parsing. Removes need to install clearview repo at RotorHazard


