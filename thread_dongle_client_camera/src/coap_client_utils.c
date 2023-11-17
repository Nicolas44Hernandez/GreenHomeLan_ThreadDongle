/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <thread_dongle_interface.h>
#include <dk_buttons_and_leds.h>
#include <net/coap_utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket.h>
#include <openthread/thread.h>

#include "coap_client_utils.h"

static bool is_connected;

static struct k_work multicast_commands_work;
static struct k_work ressources_status_work;
static struct k_work send_alarm_work;
static struct k_work wifi_status_work;
static struct k_work presence_status_work;
static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

/* Options supported by the server */
static const char *const commands_option[] = { COMMANDS_URI_PATH, NULL };
static const char *const ressources_status_option[] = { RESSOURCES_URI_PATH, NULL };
static const char *const wifi_status_option[] = { WIFI_URI_PATH, NULL };
static const char *const presence_status_option[] = { PRESENCE_URI_PATH, NULL };

volatile uint8_t msg_buf[MSG_BUFF_SIZE] = {0};
uint16_t msg_len = 0;

/* Thread multicast mesh local address */
static struct sockaddr_in6 multicast_local_addr = {
     .sin6_family = AF_INET6,
     .sin6_port = htons(COAP_PORT),
     .sin6_addr.s6_addr = { 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
     .sin6_scope_id = 0U
};

// Variable for storing orchestrator server ressources */
struct server_ressources {
    bool wifi_status;
    bool presence_status;
};

static struct server_ressources srv_ressources = {
    .wifi_status = NULL,
    .presence_status = NULL,
};

static int on_commands_msg_reply(const struct coap_packet *response,
                     struct coap_reply *reply,
                     const struct sockaddr *from)
{
     ARG_UNUSED(reply);
     ARG_UNUSED(from);   

     const uint8_t *payload;
     uint16_t payload_size = 0u; 
     char *cmd_ok = "CMD:OK";

     dk_set_led_off(RESSOURCES_STATUS_MSG_LED);

     payload = coap_packet_get_payload(response, &payload_size);

     // Check if CMD:OK in payload
     if (strstr(payload, cmd_ok) != NULL) {
          printk("THREAD [DEBBUG]: commands msg reply: CMD_ACK\r\n"); 
          dk_set_led_off(COMMANDS_MSG_LED);     
     }
exit:
     return 0;
}


static void send_commands_to_server_message(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending command to server \r\n");

     int ret_coap_req = coap_send_request(
          COAP_METHOD_PUT,(const struct sockaddr *)&multicast_local_addr,
          commands_option, &msg_buf, msg_len, on_commands_msg_reply);
     
     dk_set_led_on(COMMANDS_MSG_LED);

     // Clear buffer
     msg_len = 0;
     for( int i =0; i < MSG_BUFF_SIZE; i++ ){
          msg_buf[i] = NULL;
     }     
}

static int on_ressource_status_reply(const struct coap_packet *response,
                     struct coap_reply *reply,
                     const struct sockaddr *from)
{
     const uint8_t *payload;
     uint16_t payload_size = 0u;

     ARG_UNUSED(reply);
     ARG_UNUSED(from);

     printk("THREAD [DEBBUG]: Ressource status reply received from server \r\n");     

     dk_set_led_off(RESSOURCES_STATUS_MSG_LED);

     payload = coap_packet_get_payload(response, &payload_size); 

     // Check if wifi in payload
     char* wifi_in_payload = strchr(payload, 'w');
     if(*wifi_in_payload != NULL){
          const char wifi_status_char = (char)wifi_in_payload[5];
          srv_ressources.wifi_status = wifi_status_char == '1' ? true : false;        
     }
     
     // Check if presence in payload
     char* presence_in_payload = strchr(payload, 'p');
     if(*presence_in_payload != NULL){
          const char presence_status_char = (char)presence_in_payload[4];
          srv_ressources.presence_status = presence_status_char == '1' ? true : false;  
            
     }
     print_orchestrator_server_ressources();
exit:
     return 0;
}

static void send_ressources_status_request(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending ressources status request to server \r\n");

     coap_send_request(COAP_METHOD_GET,
                 (const struct sockaddr *)&multicast_local_addr,
                 ressources_status_option, NULL, 0u, on_ressource_status_reply);
     dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
}

static void send_alarm(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending alarm to server \r\n");

     static uint8_t msg_buf[] = ALARM;
     uint16_t msg_len = sizeof(msg_buf);

     int ret_coap_req = coap_send_request(
          COAP_METHOD_PUT,(const struct sockaddr *)&multicast_local_addr,
          commands_option, &msg_buf, msg_len, on_commands_msg_reply);
     
     dk_set_led_on(COMMANDS_MSG_LED);
}


static void send_wifi_status_request(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending wifi status request to server \r\n");

     coap_send_request(COAP_METHOD_GET,
                 (const struct sockaddr *)&multicast_local_addr,
                 wifi_status_option, NULL, 0u, on_ressource_status_reply);
     dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
}

static void send_presence_status_request(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending presence status request to server \r\n");

     coap_send_request(COAP_METHOD_GET,
                 (const struct sockaddr *)&multicast_local_addr,
                 presence_status_option, NULL, 0u, on_ressource_status_reply);
     dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
                        void *user_data)
{
     if (flags & OT_CHANGED_THREAD_ROLE) {
          switch (otThreadGetDeviceRole(ot_context->instance)) {
          case OT_DEVICE_ROLE_CHILD:
          case OT_DEVICE_ROLE_ROUTER:
          case OT_DEVICE_ROLE_LEADER:
               k_work_submit(&on_connect_work);
               is_connected = true;
               break;

          case OT_DEVICE_ROLE_DISABLED:
          case OT_DEVICE_ROLE_DETACHED:
          default:
               k_work_submit(&on_disconnect_work);
               is_connected = false;
               break;
          }
     }
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
     .state_changed_cb = on_thread_state_changed
};

static void submit_work_if_connected(struct k_work *work)
{         
     if (is_connected) {
          k_work_submit(work);
     } else {
          printk("THREAD [ERROR]: Connection is broken \r\n");
     }
}

void coap_client_utils_init(ot_connection_cb_t on_connect, ot_disconnection_cb_t on_disconnect)
{

     coap_init(AF_INET6, NULL);

     k_work_init(&on_connect_work, on_connect);
     k_work_init(&on_disconnect_work, on_disconnect);
     k_work_init(&multicast_commands_work, send_commands_to_server_message);
     k_work_init(&ressources_status_work, send_ressources_status_request);
     k_work_init(&send_alarm_work, send_alarm);
     k_work_init(&wifi_status_work, send_wifi_status_request);
     k_work_init(&presence_status_work, send_presence_status_request);

     openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
     openthread_start(openthread_get_default_context());
}

void coap_client_send_commands_to_server_message(uint8_t* msg, uint16_t msg_length)
{    
     msg_len = msg_length;
     // copy message to send to buffer
     for( int i =0; i < msg_length; i++ ){
          msg_buf[i] = msg[i];
     }     
     submit_work_if_connected(&multicast_commands_work);
}

void coap_client_send_ressources_status_request(void)
{
     submit_work_if_connected(&ressources_status_work);
}

void coap_client_send_alarm(void)
{
     submit_work_if_connected(&send_alarm_work);
}

void coap_client_send_wifi_status_request(void)
{
     submit_work_if_connected(&wifi_status_work);
}

void coap_client_send_presence_status_request(void)
{
     submit_work_if_connected(&presence_status_work);
}

void print_orchestrator_server_ressources(void){
     printk("ORCHESTRATOR [DEBBUG]: Server ressources   ");
     printk("wifi: %s   ", srv_ressources.wifi_status ? "true" : "false");
     printk("presence: %s\n\r", srv_ressources.presence_status ? "true" : "false");     
}

bool get_server_wifi_status(void){
     return srv_ressources.wifi_status;
}

bool get_server_presence_status(void){
     return srv_ressources.presence_status;
}