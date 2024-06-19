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
#define KEEP_ALIVE_MSG_PERIOD_MS   10000
#define REQUEST_STATUS_PERIOD_MS   4000

// Setup for L1 and L2
static const struct gpio_dt_spec relay_1 = GPIO_DT_SPEC_GET(DT_ALIAS(relay1), gpios);
static const struct gpio_dt_spec relay_2 = GPIO_DT_SPEC_GET(DT_ALIAS(relay2), gpios);
static const struct gpio_dt_spec relay_3 = GPIO_DT_SPEC_GET(DT_ALIAS(relay3), gpios);
static const struct gpio_dt_spec relay_4 = GPIO_DT_SPEC_GET(DT_ALIAS(relay4), gpios);


bool relay1_status = false;
bool relay2_status = false;
bool relay3_status = false;
bool relay4_status = false;

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

static void set_relays_status(bool r1_status, bool r2_status, bool r3_status, bool r4_status){
    /*Set relays status*/

    gpio_pin_set_dt(&relay_1, r1_status? 1:0);
    relay1_status = r1_status;
    gpio_pin_set_dt(&relay_2, r2_status? 1:0);
    relay2_status = r2_status;
    gpio_pin_set_dt(&relay_3, r3_status? 1:0);
    relay3_status = r3_status;
    gpio_pin_set_dt(&relay_4, r4_status? 1:0);
    relay4_status = r4_status;
    printk("THREAD [DEBBUG]: Setting relays statuses  R1:%d  R2:%d  R3:%d  R4:%d \r\n",relay1_status, relay2_status, relay3_status, relay4_status);
}

static void set_single_relay_status(uint16_t relay_number, bool status){
    /*Set single relays status*/

    if(relay_number == 1){
        gpio_pin_set_dt(&relay_1, status? 1:0);
        relay1_status = status;
        return;
    }   
    if(relay_number == 2){
        gpio_pin_set_dt(&relay_2, status? 1:0);
        relay2_status = status;
        return;
    }   
    if(relay_number == 3){
        gpio_pin_set_dt(&relay_3, status? 1:0);
        relay3_status = status;
        return;
    }   
    if(relay_number == 4){
        gpio_pin_set_dt(&relay_4, status? 1:0);
        relay4_status = status;
        return;
    }    
    printk("ERROR invalid relay number: %d", relay_number);
    return;
}

static void update_power_strip_status(){

    bool r1_status = get_server_r1_status();
    bool r2_status = get_server_r2_status();
    bool r3_status = get_server_r3_status();
    bool r4_status = get_server_r4_status();

    printk("THREAD [DEBBUG] Current power strip status R1:%d  R2:%d  R3:%d  R4:%d\r\n", r1_status, r2_status, r3_status, r4_status);

    set_relays_status(r1_status, r2_status, r3_status, r4_status);
}


int main(void)
{
    int ret1;
    int ret2;
    int ret3;
    int ret4;
    
    //Check if R1, R2, R3 and R4 are ready
    if ((!device_is_ready(relay_1.port)) && (!device_is_ready(relay_2.port)) && (!device_is_ready(relay_3.port)) && (!device_is_ready(relay_4.port))){
        printk("GPIO [ERROR]: Cannot configure devices\r\n");
        return 0;
    }

    //Configure R1, R2, R3 and R4 as outputs
    ret1 = gpio_pin_configure_dt(&relay_1, GPIO_OUTPUT_ACTIVE);
    ret2 = gpio_pin_configure_dt(&relay_2, GPIO_OUTPUT_ACTIVE);
    ret3 = gpio_pin_configure_dt(&relay_3, GPIO_OUTPUT_ACTIVE);
    ret4 = gpio_pin_configure_dt(&relay_4, GPIO_OUTPUT_ACTIVE);
    if ((ret1 < 0) && (ret2 < 0) && (ret3 < 0) && (ret4 < 0)) {
        printk("GPIO [ERROR]: Cannot configure pins\r\n");
        return 0;
    }

    ret1 = dk_leds_init();
    if (ret1) {
        printk("LEDS [ERROR]: Cannot init leds, (error: %d)\r\n", ret1);
        return 0;
    }    

    //Set R1(on), R2(on), R3(on) and R4(on) initial values
    gpio_pin_set_dt(&relay_1, 1);
    gpio_pin_set_dt(&relay_2, 1);
    gpio_pin_set_dt(&relay_3, 1);
    gpio_pin_set_dt(&relay_4, 1);
    relay1_status = true;
    relay2_status = true;
    relay3_status = true;
    relay4_status = true;

    // Init thread/coap
    coap_client_utils_init(on_ot_connect, on_ot_disconnect);   

    int wait_loops_for_ka = KEEP_ALIVE_MSG_PERIOD_MS / REQUEST_STATUS_PERIOD_MS;
    int loops_count = 0;

    // Set initial relays status
    set_relays_status(true,true,true,true);
    k_msleep(1000);

    while (1) {
        coap_client_send_power_strip_status_request();
        k_msleep(REQUEST_STATUS_PERIOD_MS*0.3);
        update_power_strip_status();
        k_msleep(REQUEST_STATUS_PERIOD_MS*0.7);
        if (loops_count >=wait_loops_for_ka ){
            coap_client_send_keep_alive();   
            loops_count = 0;
        } 
        loops_count ++;
    }
    return 0;
}