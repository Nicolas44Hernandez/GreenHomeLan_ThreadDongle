/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/sys/printk.h>

#include "ot_coap_utils.h"

LOG_MODULE_REGISTER(coap_server, CONFIG_COAP_SERVER_LOG_LEVEL);

#define SLEEP_TIME_MS 500

// Callback for commands topic
static void on_commands_request(uint8_t* msg_buf, uint8_t msg_len)
{

    printk("Commands msg received: ");
    for( int i =0; i < msg_len; i++ ){
         printk("%c", msg_buf[i]);
    }
    printk("\r\n");
    dk_set_led_on(COMMANDS_MSG_LED);
    k_msleep(SLEEP_TIME_MS);
    dk_set_led_off(COMMANDS_MSG_LED);    
}

// Callback for ressources status topic
static void on_ressource_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(SLEEP_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for wifi topic
static void on_wifi_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(SLEEP_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

// Callback for presence topic
static void on_presence_status_request()
{
    dk_set_led_on(RESSOURCES_STATUS_MSG_LED);
    k_msleep(SLEEP_TIME_MS);
    dk_set_led_off(RESSOURCES_STATUS_MSG_LED);    
}

//Callback for button press
static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
    // TODO: What to do if button pressed ???
    uint32_t buttons = button_state & has_changed;
    // TODO: send test connection message
    printk("button_state: %d  has_changed: %d  \n\r", button_state,has_changed);
    if (buttons & DK_BTN1_MSK) {
        switch_wifi_status();
        switch_presence_status();
    }    
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

int main(void)
{   int ret;

    LOG_INF("Start Thread CoAP-server dongle");

    ret = ot_coap_init(
        &on_ressource_status_request, 
        &on_wifi_status_request, 
        &on_presence_status_request, 
        &on_commands_request
    );
    
    if (ret) {
        LOG_ERR("Could not initialize OpenThread CoAP");
        goto end;
    }

    ret = dk_leds_init();
    if (ret) {
        LOG_ERR("Could not initialize leds, err code: %d", ret);
        goto end;
    }

    ret = dk_buttons_init(on_button_changed);
    if (ret) {
        LOG_ERR("Cannot init buttons (error: %d)", ret);
        goto end;
    }

    openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
    openthread_start(openthread_get_default_context());

end:
    return 0;
}
