#!/bin/bash

echo Checking skdconfig is ready for production

success=0
fail_console=1
fail_uart=2

sdk_path=sdkconfig
if grep -q "HW_VERSION_IFTRON_A=y" $sdk_path; then
        echo SUCCESS: `grep "HW_VERSION_IFTRON_A" $sdk_path`;
else
        echo "FAIL: HW_VERSION_IFTRON not enabled"
        exit $uart
fi

if grep -q "CONFIG_ESP_CONSOLE_UART_NONE=y" $sdk_path; then  
	echo SUCCESS: `grep "CONFIG_ESP_CONSOLE_UART_NONE" $sdk_path`; 
else
	echo "FAIL: Console uart is not disabled" 
	exit $fail_console
fi

exit $success
