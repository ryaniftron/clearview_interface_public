#!/bin/bash

# Do a clean build or build/flash/monitor if a number is supplied for port
if [ -z "$1" ]
then
   idf.py fullclean build
else
    idf.py build flash monitor -p /dev/ttyUSB$1
fi