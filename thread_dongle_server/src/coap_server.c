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
#include <zephyr/drivers/uart.h>

#include "ot_coap_utils.h"

#define LED_ON_TIME_MS 250

// UART variables
#define SLEEP_TIME_MS   100
#define UART_RECEIVE_TIMEOUT 500000
#define START_CHAR '~'
#define END_CHAR '#'

const struct device *uart= DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t rx_buf[MSG_BUFF_SIZE] = {0};
static uint8_t rx_msg_buf[MSG_MAX_SIZE] = {0};
static uint8_t rx_offset=0;


/* Process received char from UART */
static void process_received_char(char received_char)
{
	if(received_char == START_CHAR){
		// Empty msg buffer
		for( int i =0; i < MSG_MAX_SIZE; i++ ){
			rx_msg_buf[i] = NULL;
        }
		rx_offset = 0;
		return;
	}
	if(received_char == END_CHAR){
		printk("UART [DEBBUG] : Received message:");
		for( int i =0; i < rx_offset; i++ ){
			printk("%c", rx_msg_buf[i]);
        }
		printk("\r\n");

        // Check if wifi in message received
        char* wifi_in_msg = strchr(rx_msg_buf, 'w');
        if(*wifi_in_msg != NULL){
            const char wifi_status_char = (char)wifi_in_msg[5];
            const bool new_wifi_status = wifi_status_char == '1' ? true : false;
            set_wifi_status(new_wifi_status);
        }
        
        // Check if presence in message received
        char* presence_in_msg = strchr(rx_msg_buf, 'p');
        if(*presence_in_msg != NULL){
            const char presence_status_char = (char)presence_in_msg[4];
            const bool new_presence_status = presence_status_char == '1' ? true : false;
            set_presence_status(new_presence_status);                
        }
		return;
	}
	else{
		rx_msg_buf[rx_offset]=received_char;
		rx_offset ++;
		return;
	}
} 


/*Callback for uart messages reception*/
static void on_uart_message(const struct device *uart_dev, struct uart_event *evt, void *user_data)
{   
	switch (evt->type) {				
	case UART_RX_RDY:
        for( int i =0; i < evt->data.rx.len; i++ ){
			process_received_char(evt->data.rx.buf[evt->data.rx.offset + i]);
        }
		break;		
	case UART_RX_DISABLED:
		uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), UART_RECEIVE_TIMEOUT);
		break;
	default:
		break;
	}
}

// Callback for commands topic
static void on_commands_request(uint8_t* msg_buf, uint8_t msg_len)
{

    printk("THREAD [DEBBUG]: Commands msg received: ");
    for( int i =0; i < msg_len; i++ ){
         printk("%c", msg_buf[i]);
    }
    printk("\r\n");
    dk_set_led_on(COMMANDS_MSG_LED);
    k_msleep(LED_ON_TIME_MS);
    dk_set_led_off(COMMANDS_MSG_LED);    

    int ret = uart_tx(uart, msg_buf, msg_len, SYS_FOREVER_MS);
    printk("UART [DEBBUG]: Sending message via UART\r\n");
    if (ret) {
        printk("UART [ERROR]: Impossible to send message over UART\r\n");
        return 1;
    }
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

//Callback for button press
static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
    // TODO: What to do if button pressed ???
    uint32_t buttons = button_state & has_changed;
    if (buttons & DK_BTN1_MSK) {
        //  switch_wifi_status();
        //  switch_presence_status();
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

    ret = ot_coap_init(
        &on_ressource_status_request, 
        &on_wifi_status_request, 
        &on_presence_status_request, 
        &on_commands_request
    );
    
    if (ret) {
        printk("SERVER [ERROR]: Could not initialize OpenThread CoAP\r\n");
        return 1;
    }

    ret = dk_leds_init();
    if (ret) {
        printk("LEDS [ERROR]: Could not initialize leds, err code: %d \r\n", ret);
        return 1;
    }

    if (!device_is_ready(uart)){
		printk("UART [ERROR]: Uart device not ready\r\n");
		return 1 ;
	}

    ret = dk_buttons_init(on_button_changed);
    if (ret) {
        printk("BUTTONS [ERROR]: Cannot init buttons\r\n");
        return 1;
    }

    openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
    openthread_start(openthread_get_default_context());

    // Structure to configure uart communication
    const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};

    // Configure uart0 device
    ret = uart_configure(uart, &uart_cfg);
	if (ret == -ENOSYS) {
		return -ENOSYS;
	}	

    // Register uart callback function    
    ret = uart_callback_set(uart, on_uart_message, NULL);
    if (ret) {
        printk("UART [ERROR]: Imposible to registre callback ret=%d\r\n", ret);	
        return 1;
    } 

    // Start uart receiving reception in buffer
    uart_rx_enable(uart, rx_buf, sizeof(rx_buf), UART_RECEIVE_TIMEOUT);

    // loop forever waiting for uart messages
    while (1) {        
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
