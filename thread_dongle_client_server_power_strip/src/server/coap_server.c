/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coap_utils.h"

#define LED_ON_TIME_MS 250

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

void update_server_mode(bool mode){
    set_server_mode(mode);
}

int coap_thread_server_init()
{
    int ret;
    ret = ot_coap_init(&on_power_strip_status_request);
    
    if (ret) {
        printk("THREAD SERVER [ERROR]: Init OpenThread CoAP Server\r\n: ");
        return 1;
    }

    openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
    openthread_start(openthread_get_default_context());    

    return 0;

}



