#!/bin/bash
rm sdkconfig sdkconfig.defaults
cp sdkconfig.defaults_production sdkconfig.defaults
idf.py build

./tools/check_sdk_production.sh
rval=$?
if [ $rval -ne 0 ]; then
	exit
else
	echo "pass"
fi

idf.py build
mv build/CVCM.bin build/CVCM_`cat version.txt`.bin
