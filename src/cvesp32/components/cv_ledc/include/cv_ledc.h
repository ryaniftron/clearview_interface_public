#pragma once
// LED Codes:
typedef enum  {
    led_unprogrammed, 
    led_off, // OFF = ESP32 not powered
    led_on, // ON = ESP32 powered but not doing anything
    led_breathe_fast, // Breathing Fast = connect to race network
    led_breathe_slow, // Breathing slow = connected to Wifi, not race network
    led_blink_fast, // Blinking Fast = Error, in station mode, needs dismissed
    led_blink_slow 
} CV_LED_Code_t;
void init_cv_ledc(CV_LED_Code_t initial_led_state);
void set_ledc_code(int channel, CV_LED_Code_t led_code);