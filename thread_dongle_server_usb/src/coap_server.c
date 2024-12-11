/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coap_utils.h"

#define LED_ON_TIME_MS 250
char uart_print_buff[50]; 

// Callback for commands topic
static void on_commands_request(uint8_t* msg_buf, uint8_t msg_len)
{   
    // Empty the string
    uart_print_buff[0] = '\0';

    print_uart("[LOG]: COAP - Commands msg received: ");
    for (int i = 0; i <= msg_len; i++) {
        uart_print_buff[i] = msg_buf[i];
    }
    uart_print_buff[msg_len] = '\0';
    print_uart(uart_print_buff);

    dk_set_led_on(COMMANDS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(COMMANDS_MSG_LED);  
    
}

// Callback for ressources status topic
static void on_ressource_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for wifi topic
static void on_wifi_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for presence topic
static void on_presence_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for electrical topic
static void on_electrical_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for power strip topic
static void on_power_strip_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
                    void *user_data)
{
    if (flags & OT_CHANGED_THREAD_ROLE) {
        switch (otThreadGetDeviceRole(ot_context->instance)) {
        case OT_DEVICE_ROLE_CHILD:
        case OT_DEVICE_ROLE_ROUTER:
        case OT_DEVICE_ROLE_LEADER:
            dk_set_led_on(OT_CONNECTION_LED);
            break;

        case OT_DEVICE_ROLE_DISABLED:
        case OT_DEVICE_ROLE_DETACHED:
        default:
            dk_set_led_off(OT_CONNECTION_LED);
            break;
        }
    }
}

static struct openthread_state_changed_cb ot_state_chaged_cb = { .state_changed_cb =
                                     on_thread_state_changed };


int init_coap_thread()
{
    int ret;
    ret = ot_coap_init(
        &on_ressource_status_request, 
        &on_wifi_status_request, 
        &on_presence_status_request, 
        &on_electrical_status_request, 
        &on_power_strip_status_request, 
        &on_commands_request
    );
    
    if (ret) {
        print_uart("[ERROR]: Init OpenThread CoAP\r\n: ");
        return 1;
    }

    openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
    openthread_start(openthread_get_default_context());    

    return 0;

}

