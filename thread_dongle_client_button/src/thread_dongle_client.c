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
#define SERVER_POLLING_PERIOD_MS 20000

// UART variables
#define UART_RECEIVE_TIMEOUT 500000
#define START_CHAR '~'
#define END_CHAR '#'

const struct device *uart= DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t rx_buf[MSG_BUFF_SIZE] = {0};
static uint8_t rx_msg_buf[MSG_MAX_SIZE] = {0};
static uint8_t rx_offset=0;

/*Send the server ressources status via uart*/
static void uart_send_server_ressources_status(){

    bool wifi_status = get_server_wifi_status();
    bool presence_status = get_server_presence_status();

    printk("SERVER [DEBBUG]: current ressources status  wifi:%d   presence:%d\r\n", wifi_status, presence_status);

    // msg buffer
    static uint8_t tx_buf[] =   "~wifi:0prs:0#";

    // Add wifi status
    if(wifi_status){tx_buf[6] = '1';}
    else{tx_buf[6] = '0';}

    // Add presence status
    if(presence_status){tx_buf[11] = '1';}
    else{tx_buf[11] = '0';}

    int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
    printk("UART [DEBBUG]: Sending ressources status via UART: %s\r\n", tx_buf);
    if (ret) {
        printk("UART [ERROR]: Impossible to send message over UART\r\n");
        return 1;
    }
}
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
		printk("UART [DEBBUG]: Received message: ");
		for( int i =0; i < rx_offset; i++ ){
			printk("%c", rx_msg_buf[i]);
        }
		printk("\r\n");
        printk("THREAD [DEBBUG]: Transfer received message via thread\r\n");
         
        coap_client_send_commands_to_server_message(&rx_msg_buf, rx_offset);        
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
    //printk("UART event\r\n");
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
        coap_client_send_alarm();
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
        k_msleep(SERVER_POLLING_PERIOD_MS); 
        coap_client_send_keep_alive();     
	}
    return 0;
}
