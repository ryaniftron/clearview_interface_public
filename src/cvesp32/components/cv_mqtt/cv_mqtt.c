#include "cv_mqtt.h"
#include <string.h>
#include <stdarg.h>
#include "esp_wifi.h"
#include "esp_system.h"
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

#include "cv_uart.h"
#include "cv_ledc.h"
#include "cv_utils.h"
#include "cv_wifi.h"
#include "cv_server.h"


static const char *TAG_TEST = "CV_MQTT";
static EventGroupHandle_t mqtt_event_group;
const static int CONNECTED_BIT2 = BIT0;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool _client_conn = false;
static int qos_test = 0;
#define CVMQTT_PORT 1883 
#define CVMQTT_KEEPALIVE 10  //10 seconds before it sends a keepalive msg


// CV Defines:
char device_name[20]; 
#define PROTOCOL_VERSION "cv1"
extern uint8_t desired_seat_number;
bool active_status = false;
# define MAX_TOPIC_LEN 75
# define MAX_TOPIC_HEADER_LEN 10 // should encompass "/rx/cv1/" . room for null chars is not needed
static char* attempted_hostname;

//*******************
//** Format Topic Defines **
//*******************


//Listen to these #1-#9

//1 Send command to all receivers
const char* receiver_command_all_topic_fmt = "%s/%s/cmd_all";

//2 Send command to a seat_number
const char* receiver_command_seat_topic_fmt = "%s/%s/cmd_seat/%d"; //,"seat_number");

//3 Send command to a specific receiver
const char* receiver_command_targeted_topic_fmt = "%s/%s/cmd_target/%s";//,"receiver_serial_num")

//4 Send a kick command to a specific receiver (leave the network)
const char* receiver_kick_topic_fmt = "%s/%s/kick/%s";//,"receiver_serial_num")

//5 Send an active promotion or demotion to a specific receiver
const char* receiver_active_topic_fmt = "%s/%s/active/%s";//,"receiver_serial_num")

//6 Make a request to all receivers (all receivers reply)
const char* receiver_request_all_topic_fmt = "%s/%s/req_all";

//7 Make a request to all seats at a seat number (all seats at that seat_number reply)
const char* receiver_request_seat_all_topic_fmt = "%s/%s/req_seat_all/%d";//, None)

//8 Make a request to the active seat at a seat index
// Only the active receiver at that seat replies
const char* receiver_request_seat_active_topic_fmt = "%s/%s/req_seat_active/%d";//,"seat_number")

//9 Make a request to a specific receiver
const char* receiver_request_targeted_topic_fmt = "%s/%s/req_target/%s";//,"receiver_serial_num")

//send on these #10-#15

//10 Response for all
const char* receiver_response_all_topic_fmt = "%s/%s/resp_all";

//11 Response for a seat number
const char* receiver_response_seat_topic_fmt = "%s/%s/resp_seat/%d";//, "*")

//12 Connection status for receivers
const char* receiver_connection_topic_fmt = "rxcn/%s";//, "seat_number")

//13
const char* status_static_fmt = "status_static/%s"; // receiver_serial_number

//14
const char* status_variable_fmt = "status_variable/%s"; // receiver_serial_number

//15
const char* receiver_response_target_fmt = "%s/%s/resp_target/%s"; //receiver_serial_number

//respond to these 16-18

//16 All command topic for *ESP* 
const char* receiver_command_esp_all_topic_fmt = "%s/%s/cmd_esp_all";

//17 Send command to an *ESP* at a seat_number
const char* receiver_command_esp_seat_topic_fmt = "%s/%s/cmd_esp_seat/%d"; // seat_number

//18 Send command to an *ESP* at a specific receiver
const char* receiver_command_esp_targeted_topic_fmt = "%s/%s/cmd_esp_target/%s"; // receiver_serial_num

// A type to hold mqtt_topics that are possible to use
typedef struct
{
    char* rx_cmd_all; //1
    char* rx_cmd_seat;  //2
    char* rx_cmd_targeted;  //3 
    char* rx_kick;  //4
    char* rx_active;    //5
    char* rx_req_all;   //6
    char* rx_req_seat_all;  //7
    char* rx_req_seat_active;   //8
    char* rx_req_targeted; //9
    char* rx_resp_all; //10
    char* rx_resp_seat; //11
    char* rx_conn;  //12
    char* rx_stat_static;   //13
    char* rx_stat_variable; //14
    char* rx_resp_target; //15
    char* rx_cmd_esp_all; //16
    char* rx_cmd_esp_seat; //17
    char* rx_cmd_esp_targeted; //18
} mqtt_topics;

//Allocate memory for the topic names
char rx_cmd_all_topic[MAX_TOPIC_LEN]; //1
char rx_cmd_seat_topic[MAX_TOPIC_LEN];
char rx_cmd_targeted_topic[MAX_TOPIC_LEN];
char rx_kick_topic[MAX_TOPIC_LEN];
char rx_active_topic[MAX_TOPIC_LEN];
char rx_req_all_topic[MAX_TOPIC_LEN];
char rx_req_seat_all_topic[MAX_TOPIC_LEN];
char rx_req_seat_active_topic[MAX_TOPIC_LEN];
char rx_req_targeted_topic[MAX_TOPIC_LEN];
char rx_resp_all_topic[MAX_TOPIC_LEN];
char rx_resp_seat_topic[MAX_TOPIC_LEN];
char rx_conn_topic[MAX_TOPIC_LEN];
char status_static_topic[MAX_TOPIC_LEN];
char status_variable_topic[MAX_TOPIC_LEN];
char rx_resp_targeted_topic[MAX_TOPIC_LEN];
char rx_cmd_esp_all_topic[MAX_TOPIC_LEN];
char rx_cmd_esp_seat_topic[MAX_TOPIC_LEN];
char rx_cmd_esp_targeted_topic[MAX_TOPIC_LEN]; //18

//Initialize the topics used in the struct
mqtt_topics mtopics = {
    .rx_cmd_all=rx_cmd_all_topic, //1
    .rx_cmd_seat=rx_cmd_seat_topic,
    .rx_cmd_targeted=rx_cmd_targeted_topic,
    .rx_kick=rx_kick_topic,
    .rx_active=rx_active_topic,
    .rx_req_all=rx_req_all_topic,
    .rx_req_seat_all=rx_req_seat_all_topic,
    .rx_req_seat_active=rx_req_seat_active_topic,
    .rx_req_targeted=rx_req_targeted_topic,
    .rx_resp_all=rx_resp_all_topic,
    .rx_resp_seat=rx_resp_seat_topic,
    .rx_conn=rx_conn_topic,
    .rx_stat_static=status_static_topic,
    .rx_stat_variable=status_variable_topic,
    .rx_resp_target=rx_resp_targeted_topic, //15
    .rx_cmd_esp_all=rx_cmd_esp_all_topic, //16
    .rx_cmd_esp_seat=rx_cmd_esp_seat_topic,
    .rx_cmd_esp_targeted=rx_cmd_esp_targeted_topic, //18
};


void update_mqtt_pub_seat_topics()
{
    snprintf(mtopics.rx_resp_seat,
        MAX_TOPIC_LEN,
        receiver_response_seat_topic_fmt,
        DEVICE_TYPE,
        PROTOCOL_VERSION); 
}

void update_mqtt_sub_seat_topics()
{

    snprintf(mtopics.rx_cmd_seat,
            MAX_TOPIC_LEN,
            receiver_command_seat_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            desired_seat_number);

    snprintf(mtopics.rx_req_seat_all,
            MAX_TOPIC_LEN,
            receiver_request_seat_all_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            desired_seat_number);

    snprintf(mtopics.rx_req_seat_active,
            MAX_TOPIC_LEN,
            receiver_request_seat_active_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            desired_seat_number);
            
    snprintf(mtopics.rx_cmd_esp_seat,
            MAX_TOPIC_LEN,
            receiver_command_esp_seat_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            desired_seat_number);
}

void update_mqtt_pub_topics()
{
    update_mqtt_pub_seat_topics();

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
    update_mqtt_sub_seat_topics();
    
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
    
    //todo1
    snprintf(mtopics.rx_cmd_esp_all,
            MAX_TOPIC_LEN,
            receiver_command_esp_all_topic_fmt,
            DEVICE_TYPE,
            PROTOCOL_VERSION,
            device_name);
    //todo2
    snprintf(mtopics.rx_cmd_esp_targeted,
            MAX_TOPIC_LEN,
            receiver_command_esp_targeted_topic_fmt,
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

//Subscribe the 'client' to 'topic'
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

//Unsubscribe to 'client' from 'topic'
static bool mqtt_unsubscribe_single(esp_mqtt_client_handle_t client, char* topic)
{
    int msg_id = esp_mqtt_client_unsubscribe(client,topic);
    if (msg_id == -1){
        ESP_LOGI(TAG_TEST, "sent unsubscribe unsuccessful, topic: '%s', msg_id='%d'",topic, msg_id);
        return false;
    }
    else{
        ESP_LOGI(TAG_TEST, "sent unsubscribe successful, topic: '%s', msg_id='%d'",topic, msg_id);
        return true;
    }
}

//Subscribe to all topics stored in mtopics
static bool mqtt_subscribe_to_topics(esp_mqtt_client_handle_t client) 
{
    // Any topic sub fails will result in a true return value
    bool sub_fail = false;

    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_seat); 
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_targeted);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_kick);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_active);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_seat_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_seat_active);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_targeted);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_esp_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_esp_seat);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_esp_targeted);
    return !sub_fail;
}

static bool mqtt_unsubscribe_to_seat_topics(esp_mqtt_client_handle_t client){
    bool unsub_fail = false;

    unsub_fail &= !mqtt_unsubscribe_single(client, mtopics.rx_cmd_seat); 
    unsub_fail &= !mqtt_unsubscribe_single(client, mtopics.rx_req_seat_all);
    unsub_fail &= !mqtt_unsubscribe_single(client, mtopics.rx_req_seat_active);
    unsub_fail &= !mqtt_unsubscribe_single(client, mtopics.rx_cmd_esp_seat);
    return !unsub_fail;
}

static bool mqtt_subscribe_to_seat_topics(esp_mqtt_client_handle_t client){
    bool sub_fail = false;

    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_seat); 
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_seat_all);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_req_seat_active);
    sub_fail &= !mqtt_subscribe_single(client, mtopics.rx_cmd_esp_seat);
    return !sub_fail;
}

extern bool update_subscriptions_new_seat()
{
    if (_client_conn){ //only update subscriptions if connected to broker. 
        if (!mqtt_unsubscribe_to_seat_topics(mqtt_client)) {
            ESP_LOGW("update_subs", "Unsub failure");
        }
        update_mqtt_sub_seat_topics();
        return mqtt_subscribe_to_seat_topics(mqtt_client);
    } else { //just change the stored topic names
        update_mqtt_sub_seat_topics();
        return true;
    }
}

//Publish to topic with QOS0, no retain
static void mqtt_publish_to_topic(esp_mqtt_client_handle_t client, char* topic, char* message)
{   
    char* tag = "mqtt_publish_QOS0";
    int topic_len = strlen(topic);
    if (topic_len==0) {
        ESP_LOGE(tag, "Topic name is zero length. ");
        ESP_LOGE(tag, "    ==>unsent message: '%s'", message);
        return;
    }

    esp_mqtt_client_publish(client, topic, message, 0, qos_test, 0);
}

//Publish to topic with QOS1 and retain it
static void mqtt_publish_retained(esp_mqtt_client_handle_t client, char* topic, char* message)
{   
    char* tag = "mqtt_publish_RETAINED";
    int topic_len = strlen(topic);
    if (topic_len==0) {
        ESP_LOGE(tag, "Topic name is zero length. ");
        ESP_LOGE(tag, "    ==>unsent message: '%s'", message);
        return;
    }

    esp_mqtt_client_publish(client, topic, message, 0, 1, 1);
}


static bool process_command_esp(esp_mqtt_client_handle_t client, char* cmd){
    char* TAG_proc_esp = "PROCESS_ESP_CMD";
    bool succ = false;
    //Variable will get asked more often, so check it first
    ESP_LOGI(TAG_proc_esp, "Processing ESP_CMD: %s", cmd);
    cJSON* j = cJSON_CreateObject();
    bool s = json_api_handle(cmd, j);

    if (s){
        if (j){
            // char* jc = cJSON_Print(j);
            // mqtt_publish_to_topic(client, mtopics.rx_resp_target, jc);
            // free(jc);
            char* jc = cJSON_Print(j);
            ESP_LOGI(TAG_proc_esp, "Reply: %s", jc);
            mqtt_publish_to_topic(client, mtopics.rx_resp_target, jc);
            free(jc);
            succ = true;
        } else {
            ESP_LOGW(TAG_proc_esp, "No JSON response can be created");
        }
    } else {
        ESP_LOGW(TAG_proc_esp, "No MQTT Response Sent");
    }
    return succ;
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
    
    data[data_len] = 0;
    char* ptag = "MQTT_PROCESSOR";
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

        match = "seat/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>cmd_seat/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match seat number
            long int seat_num_parsed = strtol(topic,NULL,10);
            if (seat_num_parsed == desired_seat_number){
                ESP_LOGI(ptag, "matched cmd_seat/<seat_number>");
                cvuart_send_command(data);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching seat number. Got %ld. Expected %d",seat_num_parsed,desired_seat_number );
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

        match = "esp_";
        if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>cmd_esp
            ESP_LOGD(ptag,"matched cmd_esp...");
            topic += strlen(match);
            topic_len -= strlen(match);
            
            match = "all";
            if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>cmd_all
                ESP_LOGI(ptag,"matched cmd_esp_all");
                process_command_esp(client,data);
                return true;
            } //end match_cmd_esp_all

            match = "seat/";
            if (strncmp(topic,match,strlen(match))==0) { // <topic_header>cmd_seat/
                topic += strlen(match);
                topic_len -= strlen(match);

                //match seat number
                long int seat_num_parsed = strtol(topic,NULL,10);
                if (seat_num_parsed == desired_seat_number){
                    ESP_LOGI(ptag, "matched cmd_esp_seat/<seat_number>");
                    process_command_esp(client,data);
                    return true;
                } else {
                    //TODO output an error message on an MQTT_Error Topic
                    ESP_LOGW(ptag, "Nonmatching seat number. Got %ld. Expected %d",seat_num_parsed,desired_seat_number );
                    return false; //short circuit the rest of the parser
                }
            } //end match_cmd_esp_seat/
            
            match = "target/";
            if (strncmp(topic,match,strlen(match))==0) { // <topic_header>cmd_target/
                topic += strlen(match);
                topic_len -= strlen(match);

                if (strncmp(topic,device_name,15)==0){
                    //TODO remove magic number 15
                    ESP_LOGI(ptag, "matched cmd_esp_target/<device_name>");
                    process_command_esp(client,data);
                    return true;
                } else {
                    //TODO output an error message on an MQTT_Error Topic
                    printf("dname: '%.*s'",5,device_name);
                    ESP_LOGW(ptag, "Nonmatching device name. Got %.*s. Expected %.*s",topic_len,topic, 15,device_name); //TODO magic 15
                    return false; //short circuit the rest of the parser
                }
            } //end match_cmd_esp_target/
            ESP_LOGW(ptag, "Fell through matching esp_XXXX");
        } //end match cmd_esp
    } //end match cmd_

    
    // Requests
    match = "req_";
    if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>req_
        uint8_t* dataRx = (uint8_t*) malloc(RX_BUF_SIZE+1);
        topic += strlen(match);
        topic_len -= strlen(match);
        
        match = "all";
        if (strncmp(topic,match,strlen(match))==0) {    // <topic_header>req_all
            ESP_LOGI(ptag,"matched req_all => '%s'", data); 
            int repCount = cvuart_send_report(data, dataRx);
            if (repCount > 0){
                mqtt_publish_to_topic(client, mtopics.rx_resp_target, (char* )dataRx);
                remove_ctrlchars((char*)dataRx);
            } else {
                mqtt_publish_to_topic(client, mtopics.rx_resp_target, ""); //TODO failure payload?
            }
            free(dataRx);
            return true;
        } 

        match = "seat_all/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>req_seat_all/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match seat number
            long int seat_num_parsed = strtol(topic,NULL,10);
            if (seat_num_parsed == desired_seat_number){
                ESP_LOGI(ptag, "matched req_seat_all/<seat_number> => '%s'", data);
                int repCount = cvuart_send_report(data, dataRx);
                if (repCount > 0){
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target, (char* )dataRx);
                    remove_ctrlchars((char*)dataRx);
                } else {
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target, ""); //TODO failure payload?
                }
                free(dataRx);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching seat number. Got %ld. Expected %d",seat_num_parsed,desired_seat_number );
                free(dataRx);
                return false; //short circuit the rest of the parser
            }
        } 

        match = "seat_active/";
        if (strncmp(topic,match,strlen(match))==0) { // <topic_header>req_seat_active/
            topic += strlen(match);
            topic_len -= strlen(match);

            //match seat number
            long int seat_num_parsed = strtol(topic,NULL,10);
            if (seat_num_parsed == desired_seat_number){
                ESP_LOGI(ptag, "matched req_seat_active/<seat_number>");
                int repCount = cvuart_send_report(data, dataRx);
                if (repCount > 0){
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target, (char* )dataRx);
                    remove_ctrlchars((char*)dataRx);
                } else {
                    mqtt_publish_to_topic(client, mtopics.rx_resp_target, ""); //TODO failure payload?
                }
                free(dataRx);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                ESP_LOGW(ptag, "Nonmatching seat number. Got %ld. Expected %d",seat_num_parsed,desired_seat_number );
                free(dataRx);
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
                free(dataRx);
                return true;
            } else {
                //TODO output an error message on an MQTT_Error Topic
                printf("dname: '%.*s'",5,device_name);
                ESP_LOGW(ptag, "Nonmatching device name. Got %.*s. Expected %.*s",topic_len,topic, 15,device_name); //TODO magic 15
                free(dataRx);
                return false; //short circuit the rest of the parser
            }
        } 

    }
    
    
    //if kick: esp_mqtt_client_stop(client)
    
    ESP_LOGW(ptag, "No match on TOPIC=%.*s\r\n", topic_len, topic);
    printf("TOPIC=%.*s\r\n", topic_len, topic);
    printf("DATA=%.*s\r\n", data_len, data);
    return false;
}



static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT2);
            ESP_LOGI(TAG_TEST, "Delaying 1s before sub");
            vTaskDelay(1000/ portTICK_PERIOD_MS);
            mqtt_subscribe_to_topics(client);
            ESP_LOGI(TAG_TEST, "Delaying 1s before pub");
            vTaskDelay(1000/ portTICK_PERIOD_MS);
            char* conn_msg = "1";
            mqtt_publish_retained(mqtt_client, mtopics.rx_conn,conn_msg);
            set_nvs_strval(nvs_broker_ip, attempted_hostname);
            _client_conn = true;
            // set_ledc_code(0, led_breathe_slow);
            _client_conn = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            _client_conn = false;
            ESP_LOGW(TAG_TEST, "MQTT_EVENT_DISCONNECTED");
            #if CONFIG_ENABLE_LED == 1
                set_ledc_code(0, led_breathe_fast);
            #endif//CONFIG_ENABLE_LED == 1
            _client_conn = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_TEST, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG_TEST, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG_TEST, "MQTT_EVENT_DATA");
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


extern void mqtt_app_start(const char* mqtt_hostname)
{
    update_all_mqtt_topics();
    attempted_hostname = mqtt_hostname;
    //return //-> works
    mqtt_event_group = xEventGroupCreate();
    const esp_mqtt_client_config_t mqtt_cfg = {
        .event_handle = mqtt_event_handler,
        .host = mqtt_hostname,
        .port = CVMQTT_PORT,
        .lwt_topic = mtopics.rx_conn,
        .lwt_msg = "0", //disconnected
        .lwt_retain = 1,
        .keepalive = CVMQTT_KEEPALIVE,
    }; 

    ESP_LOGI(TAG_TEST, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG_TEST, "Connecting to broker IP '%s'",mqtt_cfg.host);
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_LOGI(TAG_TEST, "client_init is init");
    ESP_LOGI(TAG_TEST, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
}

extern void mqtt_app_stop()
{
    if (mqtt_client) {
        ESP_ERROR_CHECK(esp_mqtt_client_disconnect(mqtt_client));
        ESP_ERROR_CHECK(esp_mqtt_client_stop(mqtt_client));
    } else {
        ESP_LOGE(TAG_TEST, "can't stop nonexistant mqtt_client");
    }
}


// static void get_string(char *line, size_t size)
// {
//     int count = 0;
//     while (count < size) {
//         int c = fgetc(stdin);
//         if (c == '\n') {
//             line[count] = '\0';
//             break;
//         } else if (c > 0 && c < 127) {
//             line[count] = c;
//             ++count;
//         }
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }


void cv_mqtt_init(char* chipid, uint8_t chip_len, const char* mqtt_hostname) 
{
    strncpy(device_name,chipid,chip_len);
    //printf("CHIPLEN%d\n",chip_len);
    //printf("CHIPID: %.*s\n",chip_len,chipid);
    //printf("DNAME: %.*s\n",chip_len,device_name);
    
    mqtt_app_start(mqtt_hostname);    


    //Stop client just to be safe
    //esp_mqtt_client_stop(mqtt_client);

    xEventGroupClearBits(mqtt_event_group, CONNECTED_BIT2);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG_TEST, "Note free memory: %d bytes", esp_get_free_heap_size());
    xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT2, false, true, portMAX_DELAY);
    
    ESP_LOGI(TAG_TEST, "MQTT Init is finished");
}