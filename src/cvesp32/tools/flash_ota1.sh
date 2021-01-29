#!/bin/bash
$IDF_PATH/components/app_update/otatool.py write_ota_partition --slot 1 --input build/CVCM.bin
$IDF_PATH/components/app_update/otatool.py switch_ota_partition --slot 1
idf.py monitor

