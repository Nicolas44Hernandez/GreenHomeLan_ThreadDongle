
#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>

// Function declarations
static void on_commands_request(uint8_t* msg_buf, uint8_t msg_len);
static void on_ressource_status_request(void);
static void on_wifi_status_request(void);
static void on_presence_status_request(void);
static void on_electrical_status_request(void);
static void on_power_strip_status_request();
static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context, void *user_data);
int init_coap_thread();

#endif 

