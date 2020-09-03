#!/bin/bash
if [ ./tools/check_sdk.sh ]; then
	echo "A"
else
	echo "B"
fi

idf.py build
mv build/CVCM.bin build/CVCM_`cat version.txt`.bin
