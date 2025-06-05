/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <thread_dongle_interface.h>

#include "coap_client_utils.h"

// Server polling period
#define SERVER_POLLING_PERIOD_MS   2000
#define KEEP_ALIVE_MSG_PERIOD_MS   20000

// UART variables
#define UART_RECEIVE_TIMEOUT 500000
#define START_CHAR '~'
#define END_CHAR '#'

const struct device *uart= DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t rx_buf[MSG_BUFF_SIZE] = {0};
static uint8_t rx_msg_buf[MSG_MAX_SIZE] = {0};
static uint8_t rx_offset=0;

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
        
        if(connected_to_coap_server_in_border_router()){
            coap_client_send_alarm();
        }
        else{
            coap_client_send_power_strip_switch_status_request_backup();
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
        printk("LEDS [ERROR]: Cannot init leds, (error: %d)", ret);
        return 0;
    }

    if (!device_is_ready(uart)){
		printk("UART [ERROR]: UART device not ready\r\n");
		return 1 ;
	}

    // Init thread/coap
    coap_client_utils_init(on_ot_connect, on_ot_disconnect);

    // Send first keep alive 
    coap_client_send_keep_alive();     
    k_msleep(SERVER_POLLING_PERIOD_MS*0.2); 
    bool border_router_responding = connected_to_coap_server_in_border_router(); 

    // Set client loop params
    int wait_loops_for_ka = KEEP_ALIVE_MSG_PERIOD_MS / SERVER_POLLING_PERIOD_MS;
    int loops_count = 0;

    // loop forever
    while (true) {  
        k_msleep(SERVER_POLLING_PERIOD_MS);
        border_router_responding = connected_to_coap_server_in_border_router();  

        if(!border_router_responding){
            // Leds off
            dk_set_led_off(COMMANDS_MSG_LED); 
            dk_set_led_off(RESSOURCES_STATUS_MSG_LED);
            // Get power strip status
            coap_client_send_power_strip_status_request_backup();
            k_msleep(1000);            
        }       

        if (loops_count >=wait_loops_for_ka ){
            coap_client_send_keep_alive();  
            loops_count = 0;
        }        

        loops_count ++; 
	}
    return 0;
}
