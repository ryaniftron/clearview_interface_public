#ifndef CV_UART
#define CV_UART


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"



//hardware pins
#define TXD_PIN (GPIO_NUM_17) //U2TXD
#define RXD_PIN (GPIO_NUM_16) //U2RXD
#define UART_NUM UART_NUM_1

//general uart settings
static const int RX_BUF_SIZE = 1024;
static bool _uart_is_init = false; //this is checked to make sure uart is active before sending

//cv protocol specifics
#define cv_start_char '\n'
#define cv_end_char '\r'
#define cv_csum '&'

#define lock_report_fmt sprintf


void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    //TODO see nmea_parser for better error checking here
    ESP_ERROR_CHECK(uart_param_config(UART_NUM , &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));

    _uart_is_init = true;
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

int receiveData(const char* logName, uint8_t* data, TickType_t ticks_to_wait)
{
    esp_log_level_set(logName, ESP_LOG_INFO);
    //uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    
    size_t * cached_data_len;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&cached_data_len));
    const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, ticks_to_wait);
    if (rxBytes > 0) {
        data[rxBytes] = 0;
        ESP_LOGI(logName, "Read %d bytes: '%s'", rxBytes, data);
        //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
    } else {
        ESP_LOGI(logName, "No data available in buffer\n");
    }
    free(data);
    return rxBytes;
}


static void tx_task()
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world\r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
static void tx_task2()
{
    static const char *TX_TASK_TAG = "TX_TASK2";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world2\r\n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void rx_task()
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        size_t * cached_data_len;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&cached_data_len));
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        } else {
            ESP_LOGI(RX_TASK_TAG, "No data available in buffer\n");
        }
    }
    free(data);
}

extern void cvuart_send_command(const char* data)
{
    static const char *logName = "send_command";
    if (_uart_is_init)
    {
        sendData(logName, data);
    }
    else
    {
        ESP_LOGE(logName, "cv_uart must be initialized.");
        return;
    }
    
}

void cv_uart_send_report(const char* data)
{
    static const char *logName = "send_report";

    if (_uart_is_init)
    {
        //pass
    }
    else
    {
        ESP_LOGE(logName, "cv_uart must be initialized.");
        return;
    }

    //clear rx buffer in anticipation of the data
    ESP_ERROR_CHECK(uart_flush_input(UART_NUM));

    //send the report request
    sendData(logName, data);
    TickType_t respone_wait_tics = 100 / portTICK_RATE_MS;

    //use the timeout in the uart to wait for a response


}

//this demo's the uart functionality. 
void run_cv_uart()
{
    init_uart();
    //xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL); 
    //xTaskCreate(tx_task2, "uart_tx_task2", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}

#endif //CV_UART