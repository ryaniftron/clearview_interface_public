
#define ignore_unused_function_and_vars
#ifdef ignore_unused_function_and_vars
	#pragma GCC diagnostic ignored "-Wunused-function"
	#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_wifi.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "tcpip_adapter.h"



#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

//Components

#include "cv_utils.h"
#include "cv_api.h"
#include "cv_wifi.h"


#include "cv_mqtt.h"
#if CONFIG_ENABLE_MQTT != 1
    #warning CV MQTT is disabled
#endif // CONFIG_ENABLE_MQTT != 1


#include "cv_ledc.h"
#if CONFIG_ENABLE_LED != 1
    #warning CV LED is disabled
#endif //CONFIG_ENABLE_LED != 1



#include "cv_uart.h"




// Hardware selection
#ifndef HW_ESP_WROOM_32
    #define HW_ESP_WROOM_32 1
#endif

#ifndef HW_ESP_WROOM_32D
    #define HW_ESP_WROOM_32D 2
#endif

#ifndef HW
    //#define HW HW_ESP_WROOM_32
    #define HW HW_ESP_WROOM_32
#endif


#include "cv_repartition.h"






//****************
// UART Testing
//****************

#ifndef UART_TEST_LOOP
    //uncomment this to run the test UART command periodically.
    //#define UART_TEST_LOOP 1
#endif


//*****************
//Network credentials for station mode.
//*****************




static const char *TAG = "CV_MAIN";




void app_main(void)
{





    // tcpip_adapter_init();

    CV_LED_Code_t initial_led_state = led_off;
    init_cv_ledc(initial_led_state);
    init_uart();
    start_nvs();

    alter_default_partitions();

    // char chipid[UNIQUE_ID_LENGTH];
    // get_chip_id(chipid, UNIQUE_ID_LENGTH);


    // //Start Wifi
    // start_wifi(chipid, UNIQUE_ID_LENGTH);

    // //only after in sequential wifi do we start mqtt. Give it some time
    // vTaskDelay(2000 / portTICK_PERIOD_MS);

    // #if CONFIG_ENABLE_MQTT
    //     cv_mqtt_init(chipid, UNIQUE_ID_LENGTH, desired_mqtt_broker_ip);
    // #endif //CONFIG_ENABLE_MQTT == 1



    // printf("END OF MAIN\n");
}




