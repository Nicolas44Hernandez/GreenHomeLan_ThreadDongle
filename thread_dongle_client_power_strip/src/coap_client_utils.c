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

static struct k_work send_keep_alive_work;
static struct k_work power_strip_status_work;
static struct k_work on_connect_work;
static struct k_work on_disconnect_work;


/* Options supported by the server */
static const char *const commands_option[] = { COMMANDS_URI_PATH, NULL };
static const char *const power_strip_status_option[] = { POWER_STRIP_URI_PATH, NULL };

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
     bool r1_status;
     bool r2_status;
     bool r3_status;
     bool r4_status;
};

static struct server_ressources srv_ressources = {
    .r1_status = NULL,
    .r2_status = NULL,
    .r3_status = NULL,
    .r4_status = NULL,
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

static int on_power_strip_status_reply(const struct coap_packet *response,
                     struct coap_reply *reply,
                     const struct sockaddr *from)
{
     const uint8_t *payload;
     uint16_t payload_size = 0u;

     ARG_UNUSED(reply);
     ARG_UNUSED(from);

     printk("THREAD [DEBBUG]: Power strip status reply received from server \r\n");     

     dk_set_led_off(RESSOURCES_STATUS_MSG_LED);

     payload = coap_packet_get_payload(response, &payload_size); 

     // Print payload
     printk("THREAD [DEBBUG]: Received payload: size: %d  payload ", payload_size);
		for( int i =0; i < payload_size; i++ ){
			printk("%c", payload[i]);
     }
     printk("\r\n");

     // Check if outlet in payload
     char* outlet_in_payload = strchr(payload, 'o');
     if(*outlet_in_payload != NULL){
          // Retreive relays status
          const char r1_status_char = (char)payload[8];
          const char r2_status_char = (char)payload[9];
          const char r3_status_char = (char)payload[10];
          const char r4_status_char = (char)payload[11];
          bool r1_received_status = r1_status_char == '1' ? true : false;   
          bool r2_received_status = r2_status_char == '1' ? true : false;   
          bool r3_received_status = r3_status_char == '1' ? true : false;   
          bool r4_received_status = r4_status_char == '1' ? true : false; 

          // Set new status to server ressources instance
          srv_ressources.r1_status=r1_received_status;
          srv_ressources.r2_status=r2_received_status;
          srv_ressources.r3_status=r3_received_status;
          srv_ressources.r4_status=r4_received_status;          
     }
exit:
     return 0;
}

static void send_keep_alive(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending keep alive msg to server \r\n");

     static uint8_t msg_buf[] = KEEP_ALIVE_DEVICE_ID_7;
     uint16_t msg_len = sizeof(msg_buf);

     int ret_coap_req = coap_send_request(
          COAP_METHOD_PUT,(const struct sockaddr *)&multicast_local_addr,
          commands_option, &msg_buf, msg_len, on_commands_msg_reply);
     
     dk_set_led_on(COMMANDS_MSG_LED);
}

static void send_power_strip_status_request(struct k_work *item)
{
     ARG_UNUSED(item);

     printk("THREAD [DEBBUG]: Sending power strip status request to server \r\n");

     coap_send_request(COAP_METHOD_GET,
                 (const struct sockaddr *)&multicast_local_addr,
                 power_strip_status_option, NULL, 0u, on_power_strip_status_reply);
     dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
}

bool get_server_r1_status(void){
     return srv_ressources.r1_status;
}

bool get_server_r2_status(void){
     return srv_ressources.r2_status;
}

bool get_server_r3_status(void){
     return srv_ressources.r3_status;
}

bool get_server_r4_status(void){
     return srv_ressources.r4_status;
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
     k_work_init(&send_keep_alive_work, send_keep_alive);
     k_work_init(&power_strip_status_work, send_power_strip_status_request);

     openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
     openthread_start(openthread_get_default_context());
}

void coap_client_send_keep_alive(void)
{
     submit_work_if_connected(&send_keep_alive_work);
}

void coap_client_send_power_strip_status_request(void)
{
     submit_work_if_connected(&power_strip_status_work);
}