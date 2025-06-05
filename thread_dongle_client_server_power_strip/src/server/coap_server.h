
#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>

// Function declarations
static void on_power_strip_status_request();
static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context, void *user_data);
int coap_thread_server_init();
void update_server_mode(bool mode);
#endif 

