#include "cv_ledc.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "string.h"

static CV_LED_Code_t get_ledc_code(int channel);

//This file uses two methods for controlling the LED: gpio and ledc.
//Don't use both at the same time. ledc is more capable anyways

//Adapted from examples/peripherals/ledc/main/ledc_example_main.c


bool _ledc_init = false;
const char* TAG_LEDC = "CV_LEDC";

#define LEDC_HS_TIMER            LEDC_TIMER_0
#define LEDC_HS_MODE             LEDC_HIGH_SPEED_MODE
#ifdef CONFIG_CVLED_PIN_IFTRONA
    #define LEDC_HS_CH0_GPIO         (4)
    #define LED_DRIVE 1 // 1 = sourcing from vcc. VCC-LED-RES-GND
    #warning LED PIN on GPIO4
#endif
#ifdef CONFIG_CVLED_PIN_DEVBOARD
    #define LEDC_HS_CH0_GPIO         (2)
    #warning LED PIN on GPIO2
    #define LED_DRIVE 2 // 2= sinking to GND. VCC-RES-LED-GND
#endif
#define LEDC_HS_CH0_CHANNEL      LEDC_CHANNEL_0

#define LEDC_CH_NUM              (1) //how many LED's in use 
#define LEDC_MAX_DUTY            (8191) //2 ** LEDC_TIMER_13_BIT-1 = 2^13-1
#define LEDC_MIN_DUTY            (0)

#define LEDC_FADE_TIME_FAST      (350)
#define LEDC_FADE_TIME_SLOW      (3000)

#define LEDC_BLINK_OFF_FAST_TIME (250)
#define LEDC_BLINK_ON_FAST_TIME  (500)
#define LEDC_BLINK_OFF_SLOW_TIME (1500)
#define LEDC_BLINK_ON_SLOW_TIME  (1500)

#if LED_DRIVE == 1
    #define LED_ON_DUTY          LEDC_MIN_DUTY
    #define LED_OFF_DUTY         LEDC_MAX_DUTY
#elif LED_DRIVE == 2
    #define LED_ON_DUTY          LEDC_MAX_DUTY
    #define LED_OFF_DUTY         LEDC_MIN_DUTY
#else
    #error Unknown LED_DRIVE type
#endif


CV_LED_Code_t _led_state = led_on;
bool _led_update_state = false; //if true, LED state needs changed
bool _led_task_block = false; // This is a blocker for changing state because it's midway in a task


ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
    .freq_hz = 5000,                      // frequency of PWM signal
    .speed_mode = LEDC_HS_MODE,           // timer mode
    .timer_num = LEDC_HS_TIMER,            // timer index
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
};


ledc_channel_config_t ledc_channel[LEDC_CH_NUM] = {
    {
        .channel    = LEDC_HS_CH0_CHANNEL,
        .duty       = LED_OFF_DUTY,
        .gpio_num   = LEDC_HS_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER
    },
};

extern void set_led(char* value, struct cv_api_write* cawptr){
    cawptr->success = true;
    cawptr->api_code = CV_OK;
    if (strncmp(value, "on", strlen(value))==0){
        set_ledc_code(0, led_on);
    } else if (strncmp(value, "off", strlen(value))==0){
        set_ledc_code(0, led_off);
    }else if (strncmp(value, "breathe_slow", strlen(value))==0){
        set_ledc_code(0, led_breathe_slow);
    }else if (strncmp(value, "breathe_fast", strlen(value))==0){
        set_ledc_code(0, led_breathe_fast);
    }else if (strncmp(value, "blink_slow", strlen(value))==0){
        set_ledc_code(0, led_blink_slow);
    }else if (strncmp(value, "blink_fast", strlen(value))==0){
        set_ledc_code(0, led_blink_fast);
    }else {
        ESP_LOGE(TAG_LEDC, "Unknown set_led value %s", value);
        cawptr->success = false;
        cawptr->api_code = CV_ERROR_VALUE;
    }
}

static CV_LED_Code_t get_ledc_code(int channel){
    //TODO this only reads channel 0
    return _led_state;
}

extern void get_led(struct cv_api_read* carptr, int channel){


    carptr->success = true;
    carptr->api_code = CV_OK;
    switch(get_ledc_code(channel))
    {
        case led_off:
            carptr->val="off";
            break;
        case led_on:
            carptr->val="on";
            break;
        case led_breathe_fast:
            carptr->val="breathe_fast";
            break;
        case led_breathe_slow:
            carptr->val="breathe_slow";
            break;
        case led_blink_fast:
            carptr->val="blink_fast";
            break;
        case led_blink_slow:
            carptr->val="blink_slow";
            break;
        default:
            ESP_LOGE(TAG_LEDC, "Unknown ledc code");
            carptr->success=false;
            carptr->api_code=CV_ERROR_VALUE;
    }
}

//For some of the constant states, only need to tell LED what to do once
void _change_led_state(ledc_channel_config_t led_channel_conft){
    if (_led_task_block){
        ESP_LOGW(TAG_LEDC, "LED update blocked by task. Autowaiting for completion");
        return;
    }
    if (!_led_update_state){
        ESP_LOGD(TAG_LEDC, "LED already updated after task completion");
        return;
    }

    if (_led_state == led_off){
        ESP_LOGI(TAG_LEDC, "Setting OFF duty");
        ledc_set_duty(led_channel_conft.speed_mode, led_channel_conft.channel, LED_OFF_DUTY);
        ledc_update_duty(led_channel_conft.speed_mode, led_channel_conft.channel);
    } else if (_led_state == led_on){
         ESP_LOGI(TAG_LEDC, "Setting ON duty");
        ledc_set_duty(led_channel_conft.speed_mode, led_channel_conft.channel, LED_ON_DUTY);
        ledc_update_duty(led_channel_conft.speed_mode, led_channel_conft.channel);
    } else {
        ESP_LOGI(TAG_LEDC, "No permanent state change needed to change LED mode to %i", _led_state);
    }
    _led_update_state = false;
}

// This changes what pattern the LED is doing
void set_ledc_code(int channel, CV_LED_Code_t led_code){
    #if CONFIG_ENABLE_LED
    if (led_code == led_unprogrammed){
        ESP_LOGW(TAG_LEDC, "Can't set LED to unprogrammed because it's reserved");
    } else if (led_code == led_off){
        ESP_LOGW(TAG_LEDC, "Setting LED state to reserved off state");
        _led_state = led_code;
    } else if (led_code == led_on){
        ESP_LOGW(TAG_LEDC, "Setting LED state to reserved on state");
        _led_state = led_code;
    } else {
        _led_state = led_code; //update local variable here
        ESP_LOGI(TAG_LEDC, "LED STATE: %d", led_code);
    }

    _led_update_state = true;


    ledc_channel_config_t led_channel_conft = ledc_channel[channel];
    _change_led_state(led_channel_conft); //change pin voltage if it's a fixed state
    #else
    ESP_LOGI(TAG_LEDC, "Ignoring set_ledc_code to %i", led_code);
    #endif //CONFIG_ENABLE_LED
}

void breathe_led(ledc_channel_config_t led_channel_conft){
        int fade_time;
        if (_led_state == led_breathe_fast) {
            fade_time = LEDC_FADE_TIME_FAST;
        } else if (_led_state == led_breathe_slow) {
            fade_time = LEDC_FADE_TIME_SLOW;
        } else {
            ESP_LOGE(TAG_LEDC, "Can't breathe LED with _led_state of %i", _led_state);
            return;
        }
        _led_task_block = true;

        //fade down
        ledc_set_fade_with_time(led_channel_conft.speed_mode,
                                led_channel_conft.channel, 
                                0, 
                                fade_time);

        ledc_fade_start(led_channel_conft.speed_mode,
                        led_channel_conft.channel, 
                        LEDC_FADE_WAIT_DONE);
        
        //fade up
        ledc_set_fade_with_time(led_channel_conft.speed_mode,
                                led_channel_conft.channel, 
                                LEDC_MAX_DUTY, 
                                fade_time);

        ledc_fade_start(led_channel_conft.speed_mode,
                        led_channel_conft.channel, 
                        LEDC_FADE_WAIT_DONE);
        _led_task_block = false;
}

void blink_led(ledc_channel_config_t led_channel_conft) {
    int blink_on_time, blink_off_time;
    if (_led_state == led_blink_fast) {
        blink_on_time = LEDC_BLINK_ON_FAST_TIME;
        blink_off_time = LEDC_BLINK_OFF_FAST_TIME;
    } else if (_led_state == led_blink_slow) {
        blink_on_time = LEDC_BLINK_ON_SLOW_TIME;
        blink_off_time = LEDC_BLINK_OFF_SLOW_TIME;
    } else {
        ESP_LOGE(TAG_LEDC, "Can't blink LED with _led_state of %i", _led_state);
        return;
    }
    _led_task_block = true;

    //on
    ledc_set_duty(led_channel_conft.speed_mode, 
                  led_channel_conft.channel, 
                  LED_ON_DUTY);

    ledc_update_duty(led_channel_conft.speed_mode, 
                     led_channel_conft.channel);

    vTaskDelay(blink_on_time / portTICK_PERIOD_MS);

    //off
    ledc_set_duty(led_channel_conft.speed_mode, 
                  led_channel_conft.channel, 
                  LED_OFF_DUTY);

    ledc_update_duty(led_channel_conft.speed_mode, 
                        led_channel_conft.channel);
                        
    vTaskDelay(blink_off_time / portTICK_PERIOD_MS);
    _led_task_block = false;
}

void _run_cur_led_state(ledc_channel_config_t led_channel_conft){
    if (_led_state == led_on){
        _change_led_state(led_channel_conft); 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else if (_led_state == led_off){
        _change_led_state(led_channel_conft);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else if (_led_state == led_breathe_fast || _led_state == led_breathe_slow){
        breathe_led(led_channel_conft);
    } else if (_led_state == led_blink_fast || _led_state == led_blink_slow) {
        blink_led(led_channel_conft);
    } else {
        ESP_LOGE(TAG_LEDC, "Unsupported _led_state in _run_cur_led_state");
    }
}

void _ledc_task(){
    while (true){
        _run_cur_led_state(ledc_channel[0]); // TODO multiple LED's doesn't work
    }
}

void init_cv_ledc(CV_LED_Code_t initial_led_state){
    #if CONFIG_ENABLE_LED
    if (_ledc_init){ //LED is already initialized

        ESP_LOGW(TAG_LEDC, "CV_LEDC has already been initialized");
        return;
    } else { //LED isn't initialized yet. Do it
        ledc_timer_config(&ledc_timer);

        // Set LED Controller with previously prepared configuration
        for (int ch = 0; ch < LEDC_CH_NUM; ch++) {
            ledc_channel_config(&ledc_channel[ch]);
        }

        // Initialize fade service.
        ledc_fade_func_install(0);

        _ledc_init = true;

        ESP_LOGI(TAG_LEDC, "CV_LEDC initialized. Starting xTask for LEDC");
        xTaskCreate(_ledc_task, "ledc_task", 1024*4, NULL, configMAX_PRIORITIES, NULL);
    }
    #else
    ESP_LOGW(TAG_LEDC, "CV_LEDC Init Ignored; LED is disabled");
    #endif //CONFIG_ENABLE_LED
}


void demo_ledc_codes(){
    set_ledc_code(0, led_breathe_fast);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    set_ledc_code(0, led_breathe_slow);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    set_ledc_code(0, led_blink_fast);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    set_ledc_code(0, led_blink_slow);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    set_ledc_code(0, led_on);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    set_ledc_code(0, led_off);

}