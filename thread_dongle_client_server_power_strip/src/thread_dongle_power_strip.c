/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/printk.h>
#include <thread_dongle_interface.h>

#include "client/coap_client_utils.h"
#include "server/coap_server.h"
#include "relays/relays_utils.h"

#define KEEP_ALIVE_MSG_PERIOD_MS   10000
#define REQUEST_STATUS_PERIOD_MS   2000
#define SERVER_RECONNECTION_PERIOD_MS   15000

bool server_mode = false;

static void on_ot_connect(struct k_work *item)
{
    ARG_UNUSED(item);
    dk_set_led_on(OT_CONNECTION_LED);
}

static void on_ot_disconnect(struct k_work *item)
{
    ARG_UNUSED(item);
    dk_set_led_off(OT_CONNECTION_LED);
}

static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
    uint32_t buttons = button_state & has_changed;
    if (buttons & DK_BTN1_MSK) {
        server_mode = !server_mode;
    }
}

void run_coap_service(){

    // Init thread/coap Server
    coap_thread_server_init();

    // Init thread/coap Client
    coap_client_utils_init(on_ot_connect, on_ot_disconnect); 

    // Set client loop params
    int wait_loops_for_ka = KEEP_ALIVE_MSG_PERIOD_MS / REQUEST_STATUS_PERIOD_MS;
    int loops_count = 0;

    k_msleep(1000);

    while(true){
        if(server_mode) {
            printk("THREAD SERVER [INFO]: Trying to reconnect to server\r\n");
            coap_client_send_power_strip_status_request();
            k_msleep(SERVER_RECONNECTION_PERIOD_MS);    
            if (connected_to_coap_server()){
                printk("THREAD MODE [INFO]: SWITCHING TO CLIENT MODE\r\n");
                server_mode = false;
                update_server_mode(false);
                // Leds off
                dk_set_led_off(COMMANDS_MSG_LED); 
                dk_set_led_off(RESSOURCES_STATUS_MSG_LED); 
                continue;
            }           
        }
        else{
            coap_client_send_power_strip_status_request();
            k_msleep(REQUEST_STATUS_PERIOD_MS);    
            if (!connected_to_coap_server()){
                printk("THREAD MODE [INFO]: SWITCHING TO SERVER MODE\r\n");
                server_mode = true;
                update_server_mode(true);
                // Leds off
                dk_set_led_off(COMMANDS_MSG_LED); 
                dk_set_led_off(RESSOURCES_STATUS_MSG_LED); 
                continue;
            }    
            if (loops_count >=wait_loops_for_ka ){
                coap_client_send_keep_alive();   
                loops_count = 0;
            }         
            loops_count ++; 
        }
        
    }
}

int main(void)
{
    int ret;

    ret = dk_buttons_init(on_button_changed);
    if (ret) {
        printk("BUTTONS [ERROR]: Cannot init buttons (error: %d)", ret);
        return 0;
    }

    ret = dk_leds_init();
    if (ret) {
        printk("LEDS [ERROR]: Cannot init leds, (error: %d)\r\n", ret);
        return 0;
    }    

    ret = init_relays();
    if (ret) {
        printk("RELAYS [ERROR]: Cannot init relays (error: %d)", ret);
        return 0;
    }

    // Init thread / CoAP client
    run_coap_service();

    return 0;
}