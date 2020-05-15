#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

bool _led_init = false;
const char* TAG_LED = "CV_LED_GPIO";
#ifndef LED_DRIVE
    #define LED_DRIVE 1 //1 for sourcing from vcc, 2 for sinking to gnd
#endif

#ifndef DONGLE_LED_PIN
#define DONGLE_LED_PIN (GPIO_NUM_4)
#endif



void init_gpio_led(){
    if (_led_init){ //LED is already initialized
        ESP_LOGW(TAG_LED, "CV_LED has already been initialized");
        return;
    } else { //LED isn't initialized yet. Do it
        
        gpio_pad_select_gpio(DONGLE_LED_PIN);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(DONGLE_LED_PIN, GPIO_MODE_OUTPUT);
        _led_init = true;
        ESP_LOGI(TAG_LED, "CV_LED initialized");
    }
}

void turn_on_gpio_led(){
    ESP_LOGD(TAG_LED, "LED ON");
    if (!_led_init){ init_led();}

    #if LED_DRIVE == 1
    /* Blink on (output low) */
    gpio_set_level(DONGLE_LED_PIN, 0);
    #else
    /* Blink on (output high) */
    gpio_set_level(DONGLE_LED_PIN, 1);
    #endif
    return;
}

void turn_off_gpio_led(){
    ESP_LOGD(TAG_LED, "LED OFF");
    if (!_led_init){ init_led();}

    #if LED_DRIVE == 1
    /* Blink off (output high) */
    gpio_set_level(DONGLE_LED_PIN, 1);
    #else
    /* Blink off (output low) */
    gpio_set_level(DONGLE_LED_PIN, 0);
    #endif
    return;
}

void run_gpio_led_blink(){

    while(1) {
        turn_on_led();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        /* Blink off (output high) */
        turn_off_led();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

