/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2016-2023 Makerdiary
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>

#include "coap_server.h"
#include "uart_interface.h"

#define SLEEP_TIME_MS   100

//Callback for button press
static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
    // TODO: What to do if button pressed ???
    uint32_t buttons = button_state & has_changed;
    if (buttons & DK_BTN1_MSK) {
        print_ressources_status();
        //  switch_wifi_status();
        //  switch_presence_status();
        //  switch_electrical_status();
    }    
}

int main(void)
{   int ret;

    ret = init_uart();
    if (ret) {
        print_uart("[ERROR]: UART init \r\n");
        return 1;
    } 
    print_uart("[LOG]: UART initialized\r\n");

    print_uart("[LOG]: Green WiFi Thread Dongle .\r\n");
	print_uart("[LOG]: Developed by Nicolas Hernandez\r\n");
    print_uart("[LOG]: nicolas.hernandez@orange.com\r\n");

    ret = dk_leds_init();
    if (ret) {
        print_uart("[ERROR]: leds init \r\n");
        return 1;
    }
    print_uart("[LOG]: Leds initialized\r\n");

    ret = dk_buttons_init(on_button_changed);
    if (ret) {
        print_uart("[ERROR]: buttons init \r\n");
        return 1;
    }
    print_uart("[LOG]: Butttons initialized\r\n");

    ret = init_coap_thread();
    if (ret) {
        print_uart("[ERROR]: Thread init \r\n");
        return 1;
    }    
    print_uart("[LOG]: Thread initialized\r\n");

    // loop forever waiting for uart messages
    read_forever();
    return 0;
}
