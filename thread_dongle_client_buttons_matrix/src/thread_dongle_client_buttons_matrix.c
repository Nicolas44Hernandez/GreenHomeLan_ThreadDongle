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

#include "coap_client_utils.h"

// Buttons pressed check period
#define BUTTONS_PRESS_PERIOD_MS   100
#define TOGGLE_DEBOUNCE_PERIOD_MS 10

// Setup for L1 and L2
static const struct gpio_dt_spec line_1 = GPIO_DT_SPEC_GET(DT_ALIAS(line1), gpios);
static const struct gpio_dt_spec line_2 = GPIO_DT_SPEC_GET(DT_ALIAS(line2), gpios);

// Setup for L1 and L2
static const struct gpio_dt_spec row_1 = GPIO_DT_SPEC_GET(DT_ALIAS(row1), gpios);
static const struct gpio_dt_spec row_2 = GPIO_DT_SPEC_GET(DT_ALIAS(row2), gpios);

bool l1_status = false;
bool l2_status = false;

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


static void toggle_matrix_lines()
{   
    bool l1_old_status = l1_status;
    bool l2_old_status = l2_status;
    gpio_pin_set_dt(&line_1, l1_old_status? 1:0);
    l1_status = !l1_old_status;
    gpio_pin_set_dt(&line_2, l2_old_status? 1:0);
    l2_status = !l2_old_status;
    //printk("L1:%d  L2:%d \r\n",l1_status,l2_status);
    k_msleep(TOGGLE_DEBOUNCE_PERIOD_MS);
} 

static void check_buttons()
{     
    bool row_1_status = gpio_pin_get_dt(&row_1);
    bool row_2_status = gpio_pin_get_dt(&row_2);
    
    if(row_1_status){ 
        dk_set_led_on(RESSOURCES_STATUS_MSG_LED); 
        if (l1_status){
            // S1 pressed
            coap_client_send_command_to_server_message(1);
        }
        else{
            // S2 pressed
            coap_client_send_command_to_server_message(2);
        }        
        k_msleep(250);
        dk_set_led_off(RESSOURCES_STATUS_MSG_LED); 
        return;
    }
    if(row_2_status){ 
        dk_set_led_on(RESSOURCES_STATUS_MSG_LED);         
        if (l1_status){
            // S3 pressed
            coap_client_send_command_to_server_message(3);
        }
        else{
            // S4 pressed
            coap_client_send_command_to_server_message(4);
        }        
        k_msleep(250);
        dk_set_led_off(RESSOURCES_STATUS_MSG_LED); 
        return;
    }
} 

int main(void)
{
    int ret1;
    int ret2;
    
    //Check if L1 L2, R1 and R2 are ready
    if ((!device_is_ready(line_1.port)) && (!device_is_ready(line_2.port)) && (!device_is_ready(row_1.port)) && (!device_is_ready(row_2.port))){
        printk("GPIO [ERROR]: Cannot configure devices\r\n");
        return 0;
    }

    //Configure L1 and L2 as input/outputs
    ret1 = gpio_pin_configure_dt(&line_1, GPIO_OUTPUT_ACTIVE);
    ret2 = gpio_pin_configure_dt(&line_2, GPIO_OUTPUT_ACTIVE);
    if ((ret1 < 0) && (ret2 < 0)) {
        printk("GPIO [ERROR]: Cannot configure pins\r\n");
        return 0;
    }

    ret1 = dk_leds_init();
    if (ret1) {
        printk("LEDS [ERROR]: Cannot init leds, (error: %d)\r\n", ret1);
        return 0;
    }

    // Configure R1 and R2 as inputs
    ret1 = gpio_pin_configure_dt(&row_1, GPIO_INPUT);
    ret2 = gpio_pin_configure_dt(&row_2, GPIO_INPUT);
	if ((ret1 < 0) && (ret2 < 0)) {
		return;
	}

    //Set L1(on) and L2(off) initial values
    gpio_pin_set_dt(&line_1, 1);
    gpio_pin_set_dt(&line_2, 0);
    l1_status = false;
    l2_status = true;

    // Init thread/coap
    coap_client_utils_init(on_ot_connect, on_ot_disconnect);   

    while (1) {
        k_msleep(BUTTONS_PRESS_PERIOD_MS);
        toggle_matrix_lines();
        check_buttons();  
    }
    return 0;
}