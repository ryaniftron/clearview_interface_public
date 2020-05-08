#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cv_uart.c" 
#include "cv_ledc.c"

static const char *TAG_TEST = "PUBLISH_TEST";
static EventGroupHandle_t mqtt_event_group;
const static int CONNECTED_BIT2 = BIT0;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char *expected_data = NULL;
static char *actual_data = NULL;
static size_t expected_size = 0;
static size_t expected_published = 0;
static size_t actual_published = 0;
static int qos_test = 0;



// CV Defines
char device_name[20]; 
#define DEVICE_TYPE "rx"
#define PROTOCOL_VERSION "cv1"
int node_number = 0;
bool active_status = false;
# define MAX_TOPIC_LEN 75
# define MAX_TOPIC_HEADER_LEN 10 // should encompass "/rx/cv1/" . room for null chars is not needed

//*******************
//** Format Topic Defines **
//*******************

//1 Send command to all receivers
const char* receiver_command_all_topic_fmt = "%s/%s/cmd_all";

//2 Send command to a node_number
const char* receiver_command_node_topic_fmt = "%s/%s/cmd_node/%d"; //,"node_number");

//3 Send command to a specific receiver
const char* receiver_command_targeted_topic_fmt = "%s/%s/cmd_target/%s";//,"receiver_serial_num")

//4 Send a kick command to a specific receiver (leave the network)
const char* receiver_kick_topic_fmt = "%s/%s/kick/%s";//,"receiver_serial_num")

//5 Send an active promotion or demotion to a specific receiver
const char* receiver_active_topic_fmt = "%s/%s/active/%s";//,"receiver_serial_num")

//6 Make a request to all receivers (all receivers reply)
const char* receiver_request_all_topic_fmt = "%s/%s/req_all";

//7 Make a request to all nodes at a node number (all nodes at that node_number reply)
const char* receiver_request_node_all_topic_fmt = "%s/%s/req_node_all/%d";//, None)

//8 Make a request to the active node at a node index
// Only the active receiver at that node replies
const char* receiver_request_node_active_topic_fmt = "%s/%s/req_node_active/%d";//,"node_number")

//9 Make a request to a specific receiver
const char* receiver_request_targeted_topic_fmt = "%s/%s/req_target/%s";//,"receiver_serial_num")

//        # Subscribe Topics

//10 Response for all
const char* receiver_response_all_topic_fmt = "%s/%s/resp_all";

//11 Response for a node number
const char* receiver_response_node_topic_fmt = "%s/%s/resp_node/%d";//, "*")

//12 Connection status for receivers
const char* receiver_connection_topic_fmt = "rxcn/%s";//, "node_number")

//13
const char* status_static_fmt = "status_static/%s"; // receiver_serial_number

//14
const char* status_variable_fmt = "status_variable/%s"; // receiver_serial_number

//15
const char* receiver_response_target_fmt = "%s/%s/resp_target/%s"; //receiver_serial_number

typedef struct
{
    char* rx_cmd_all; //1
    char* rx_cmd_node;  //2
    char* rx_cmd_targeted;  //3 
    char* rx_kick;  //4
    char* rx_active;    //5
    char* rx_req_all;   //6
    char* rx_req_node_all;  //7
    char* rx_req_node_active;   //8
    char* rx_req_targeted; //9
    char* rx_resp_all; //10
    char* rx_resp_node; //11
    char* rx_conn;  //12
    char* rx_stat_static;   //13
    char* rx_stat_variable; //14
    char* rx_resp_target; //15
} mqtt_topics;

char rx_cmd_all_topic[MAX_TOPIC_LEN];
char rx_cmd_node_topic[MAX_TOPIC_LEN];
char rx_cmd_targeted_topic[MAX_TOPIC_LEN];
char rx_kick_topic[MAX_TOPIC_LEN];
char rx_active_topic[MAX_TOPIC_LEN];
char rx_req_all_topic[MAX_TOPIC_LEN];
char rx_req_node_all_topic[MAX_TOPIC_LEN];
char rx_req_node_active_topic[MAX_TOPIC_LEN];
char rx_req_targeted_topic[MAX_TOPIC_LEN];
char rx_resp_all_topic[MAX_TOPIC_LEN];
char rx_resp_node_topic[MAX_TOPIC_LEN];
char rx_conn_topic[MAX_TOPIC_LEN];
char status_static_topic[MAX_TOPIC_LEN];
char status_variable_topic[MAX_TOPIC_LEN];
char rx_resp_targeted_topic[MAX_TOPIC_LEN];

mqtt_topics mtopics = {
    .rx_cmd_all=rx_cmd_all_topic,
    .rx_cmd_node=rx_cmd_node_topic,
    .rx_cmd_targeted=rx_cmd_targeted_topic,
    .rx_kick=rx_kick_topic,
    .rx_active=rx_active_topic,
    .rx_req_all=rx_req_all_topic,
    .rx_req_node_all=rx_req_node_all_topic,
    .rx_req_node_active=rx_req_node_active_topic,
    .rx_req_targeted=rx_req_targeted_topic,
    .rx_resp_all=rx_resp_all_topic,
    .rx_resp_node=rx_resp_node_topic,
    .rx_conn=rx_conn_topic,
    .rx_stat_static=status_static_topic,
    .rx_stat_variable=status_variable_topic,
    .rx_resp_target=rx_resp_targeted_topic,
};





void update_mqtt_pub_node_topics()
{
    snprintf(mtopics.rx_resp_node,
        MAX_TOPIC_LEN,
        receiver_response_node_topic_fmt,
        DEVICE_TYPE,
        PROTOCOL_VERSION); 
}

void update_mqtt_sub_node_topics()
{

    snprintf(mtopics.rx_cmd_node,
            MAX_TOPIC_LEN,
            receiver_command_node_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            node_number);

    snprintf(mtopics.rx_req_node_all,
            MAX_TOPIC_LEN,
            receiver_request_node_all_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            node_number);

    snprintf(mtopics.rx_req_node_active,
            MAX_TOPIC_LEN,
            receiver_request_node_active_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            node_number);
}

void update_mqtt_pub_topics()
{
    update_mqtt_pub_node_topics();

    snprintf(mtopics.rx_conn,
            MAX_TOPIC_LEN,
            receiver_connection_topic_fmt,
            device_name);

    snprintf(mtopics.rx_stat_static,
            MAX_TOPIC_LEN,
            status_static_fmt,
            device_name);

    snprintf(mtopics.rx_stat_variable,
            MAX_TOPIC_LEN,
            status_variable_fmt,
            device_name);
    
    snprintf(mtopics.rx_resp_target,
            MAX_TOPIC_LEN,
            receiver_response_target_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            device_name);
            
}

void update_mqtt_sub_topics()
{
    update_mqtt_sub_node_topics();
    
    snprintf(mtopics.rx_cmd_all, 
            MAX_TOPIC_LEN, 
            receiver_command_all_topic_fmt, 
            DEVICE_TYPE,
            PROTOCOL_VERSION);

    snprintf(mtopics.rx_cmd_targeted,
            MAX_TOPIC_LEN,
            receiver_command_targeted_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            device_name);

    snprintf(mtopics.rx_kick,
            MAX_TOPIC_LEN,
            receiver_kick_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            device_name);
    
    snprintf(mtopics.rx_active,
            MAX_TOPIC_LEN,
            receiver_active_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION ,
            device_name);

    snprintf(mtopics.rx_req_all,
            MAX_TOPIC_LEN,
            receiver_request_all_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION );

    snprintf(mtopics.rx_req_targeted,
            MAX_TOPIC_LEN,
            receiver_request_targeted_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            device_name);
}

// Populate the mqtt_topic objects with the correct values
void update_all_mqtt_topics(){
    update_mqtt_pub_topics();
    update_mqtt_sub_topics();
}

// https://stackoverflow.com/questions/18248047/allocate-memory-for-a-struct-with-a-character-pointer-in-c
// http://www.cplusplus.com/reference/cstdio/vsprintf/
// https://stackoverflow.com/questions/1056411/how-to-pass-variable-number-of-arguments-to-printf-sprintf


static bool mqtt_subscribe_single(esp_mqtt_client_handle_t client, char* topic){
    int msg_id = esp_mqtt_client_subscribe(client,topic , qos_test);
    if (msg_id == -1){
        ESP_LOGI(TAG_TEST, "sent subscribe unsuccessful, topic: '%s', msg_id='%d'",topic, msg_id);
        return false;
    }
    else{
        ESP_LOGI(TAG_TEST, "sent subscribe successful, topic: '%s', msg_id='%d'",topic, msg_id);
        return true;
    }
}

static bool mqtt_unsubscribe_single(esp_mqtt_client_handle_t client, char* topic)
{
    int msg_id = esp_mqtt_client_unsubscribe(client,topic);
    if (msg_id == -1){
        ESP_LOGI(TAG_TEST, "sent subscribe unsuccessful, topic: '%s', msg_id='%d'",topic, msg_id);
        return false;
    }
    else{
        ESP_LOGI(TAG_TEST, "sent subscribe successful, topic: '%s', msg_id='%d'",topic, msg_id);
        return true;
    }
}

static bool mqtt_subscribe_to_topics(esp_mqtt_client_handle_t client) 
{
    // Any topic sub fails will result in a true return value
    bool sub_fail = false;

    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_node); 
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_targeted);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_kick);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_active);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_node_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_node_active);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_targeted);
    return !sub_fail;
}

static void mqtt_publish_to_topic(esp_mqtt_client_handle_t client, char* topic, char* message)
{   
    char* tag = "mqtt_publish_to_topic";
    int topic_len = strlen(topic);
    if (topic_len==0) {
        ESP_LOGE(tag, "Topic name is zero length. ");
        ESP_LOGE(tag, "    ==>unsent message: '%s'", message);
        return;
    }

    esp_mqtt_client_publish(client, topic, message, 0, qos_test, 0);
}


//Parses a message. 
//   * Command Topics
//        * * Writes the message to UART
//   * Request Topics
//        * Writes the message to UART and reads the reply
//        * Send the reply payload back via MQTT
// Return:
//     True if parse of incoming topic matched an expected topic
//     False if the MQTT message did not match
// Note: 
static bool process_mqtt_message(esp_mqtt_client_handle_t client, int topic_len,char* topic, int data_len, char* data)
{
    printf("Xdname: '%.*s'",5,device_name);
    printf("TOPIC=%.*s\r\n", topic_len, topic);
    printf("DATA=%.*s\r\n", data_len, data);
    data[data_len] = 0;
    char* ptag = "process_parser";
    esp_log_level_set(ptag,ESP_LOG_DEBUG);

    //Check the topic header "rx/cv1" for a match
    char topic_header[MAX_TOPIC_HEADER_LEN];
    const char* topic_header_fmt = "%s/%s/";
    int topic_header_len = snprintf(topic_header,
                                    MAX_TOPIC_HEADER_LEN,
                                    topic_header_fmt,
                                    DEVICE_TYPE,
                                    PROTOCOL_VERSION);

    if (strncmp(topic_header,topic,topic_header_len)){
        ESP_LOGI(ptag, "No topic header match of %s",topic_header);
        return false;
    }
    // Valid topic header, so remove the header and continue parsing
    topic += topic_header_len;
    topic_len -= topic_header_len;

    //printf("TopicMoved=%.*s\r\n", topic_len-topic_header_len, topic);

    char* match;

    //Commands
    match = "cmd_";
    if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>cmd_
        topic += strlen(match);
        topic_len -= strlen(match);
        
        match = "all";
        if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>cmd_all
            ESP_LOGI(ptag,"matched cmd_all");
            cvuart_send_command(data);
            return true;
        } 

        match = "node/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>cmd_node/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match node number
            long int node_num_parsed = strtol(topic,NULL,10);
            if (node_num_parsed == node_number){
                ESP_LOGI(ptag, "matched cmd_node/<node_number>");
                cvuart_send_command(data);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching node number. Got %ld. Expected %d",node_num_parsed,node_number );
                return false; //short circuit the rest of the parser
            }

        } 
        
        match = "target/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>cmd_target/
            topic += strlen(match);
            topic_len -= strlen(match);

            if (strncmp(topic,device_name,15)==0){
                //TODO remove magic number 15
                ESP_LOGI(ptag, "matched cmd_target/<device_name>");
                cvuart_send_command(data);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                printf("dname: '%.*s'",5,device_name);
                ESP_LOGW(ptag, "Nonmatching device name. Got %.*s. Expected %.*s",topic_len,topic, 15,device_name); //TODO magic 15
                return false; //short circuit the rest of the parser
            }
        } 
    } //end match cmd_

    
    // Requests
    match = "req_";
    if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>req_
        uint8_t* dataRx = (uint8_t*) malloc(RX_BUF_SIZE+1);
        topic += strlen(match);
        topic_len -= strlen(match);
        
        match = "all";
        if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>req_all
            ESP_LOGI(ptag,"matched req_all");
            const int bytesRead = cvuart_send_report(data, dataRx);
            if (bytesRead > 0){
                mqtt_publish_to_topic(client, mtopics.rx_resp_target, (char* )dataRx); //TODO reply by all topic?
            }
            return true;
        } 

        match = "node_all/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>req_node_all/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match node number
            long int node_num_parsed = strtol(topic,NULL,10);
            if (node_num_parsed == node_number){
                ESP_LOGI(ptag, "matched req_node_all/<node_number>");
                const int bytesRead = cvuart_send_report(data, dataRx);
                if (bytesRead > 0){
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target,(char* ) dataRx); //TODO reply by node topic?
                }
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching node number. Got %ld. Expected %d",node_num_parsed,node_number );
                return false; //short circuit the rest of the parser
            }
        } 

        match = "node_active/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>req_node_active/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match node number
            long int node_num_parsed = strtol(topic,NULL,10);
            if (node_num_parsed == node_number){
                ESP_LOGI(ptag, "matched req_node_active/<node_number>");
                const int bytesRead = cvuart_send_report(data, dataRx);
                if (bytesRead > 0){
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target,(char* ) dataRx); //TODO reply by active node topic?
                }
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching node number. Got %ld. Expected %d",node_num_parsed,node_number );
                return false; //short circuit the rest of the parser
            }
        } 
         
        match = "target/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>req_target/
            topic += strlen(match);
            topic_len -= strlen(match);

            if (strncmp(topic,device_name,15)==0){
                //TODO remove magic number 15
                ESP_LOGI(ptag, "matched req_target/<device_name>");
                const int bytesRead = cvuart_send_report(data, dataRx);
                if (bytesRead > 0){
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target, (char* ) dataRx);
                }
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                printf("dname: '%.*s'",5,device_name);
                ESP_LOGW(ptag, "Nonmatching device name. Got %.*s. Expected %.*s",topic_len,topic, 15,device_name); //TODO magic 15
                return false; //short circuit the rest of the parser
            }
        } 

        
        free(dataRx); //TODO if I return here, then isn't there a memory leak?
    }
    
    
    //if kick: esp_mqtt_client_stop(client)
    
    ESP_LOGW(ptag, "No match on TOPIC=%.*s\r\n", topic_len, topic);
    return false;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    static int msg_id = 0;
    static int actual_len = 0;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT2);
            mqtt_subscribe_to_topics(client);

            char* conn_msg = "1";
            mqtt_publish_to_topic(mqtt_client, mtopics.rx_conn,conn_msg);
            set_ledc_code(0, led_breathe_slow);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG_TEST, "MQTT_EVENT_DISCONNECTED");
            set_ledc_code(0, led_breathe_fast);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_DATA");
            //TODO the message may be broken up over multiple pieces of data
            if (!process_mqtt_message(client, event->topic_len,event->topic, event->data_len, event->data)) {
                ESP_LOGI(TAG_TEST, "Failed to process");
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_ERROR");
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        default:
            ESP_LOGI(TAG_TEST, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_app_start(const char* mqtt_hostname)
{
    update_all_mqtt_topics();
    //return //-> works
    mqtt_event_group = xEventGroupCreate();
    const esp_mqtt_client_config_t mqtt_cfg = {
        .event_handle = mqtt_event_handler,
        .host = mqtt_hostname,
        .port = 1883,
        .lwt_topic = mtopics.rx_conn,
        .lwt_msg = "0", //disconnected
    }; 

    ESP_LOGI(TAG_TEST, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_TEST, "Connecting to broker IP %s",mqtt_cfg.host );
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
}



static void get_string(char *line, size_t size)
{
    int count = 0;
    while (count < size) {
        int c = fgetc(stdin);
        if (c == '\n') {
            line[count] = '\0';
            break;
        } else if (c > 0 && c < 127) {
            line[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void cv_mqtt_init(char* chipid, uint8_t chip_len, const char* mqtt_hostname) 
{
    printf("Starting MQTT\n");
    strncpy(device_name,chipid,chip_len);
    //printf("CHIPLEN%d\n",chip_len);
    //printf("CHIPID: %.*s\n",chip_len,chipid);
    //printf("DNAME: %.*s\n",chip_len,device_name);
    
    mqtt_app_start(mqtt_hostname);

    init_uart();


    //Stop client just to be safe
    //esp_mqtt_client_stop(mqtt_client);

    xEventGroupClearBits(mqtt_event_group, CONNECTED_BIT2);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG_TEST, "Note free memory: %d bytes", esp_get_free_heap_size());
    xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT2, false, true, portMAX_DELAY);
    
}
        