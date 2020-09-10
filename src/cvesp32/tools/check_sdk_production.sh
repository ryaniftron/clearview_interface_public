#!/bin/bash

echo "Checking sdkconfig is ready for production"

success=0
fail_console=1
fail_uart=2
fail_no_sdk=3
fail_led_wrong=4
fail_hw=5

sdk_path=sdkconfig

if [ -f tools/$sdk_path ]; then
        echo "FAIL: No sdkconfig found in " `pwd` "/tools/" $sdk_path 
        exit $fail_no_sdk
fi


if grep -q "HW_VERSION_IFTRON_A=y" $sdk_path; then
        echo SUCCESS: `grep "HW_VERSION_IFTRON_A" $sdk_path`;
else
        echo "FAIL: HW_VERSION_IFTRON not enabled"
        exit $fail_hw
fi

if grep -q "CONFIG_ESP_CONSOLE_UART_NONE=y" $sdk_path; then  
	echo SUCCESS: `grep "CONFIG_ESP_CONSOLE_UART_NONE" $sdk_path`; 
else
	echo "FAIL: Console uart is not disabled" 
	exit $fail_console
fi

if grep -q "CONFIG_CVLED_PIN_IFTRONA=y" $sdk_path; then  
	echo SUCCESS: `grep "CONFIG_CVLED_PIN_IFTRONA" $sdk_path`; 
else
	echo "FAIL: LED is set for development, not iftronA" 
	exit $fail_led_wrong
fi

exit $success
