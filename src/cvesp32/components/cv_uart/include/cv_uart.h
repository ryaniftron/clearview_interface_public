#pragma once
#include "esp_system.h"

#define RX_BUF_SIZE 128
#define LOCK_REPORT_FMT "\n09RPLF\%\r"

extern void init_uart(void);
extern bool cvuart_send_command(const char* data);
int cvuart_send_report(const char* data, uint8_t* dataRx);