/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "uart_interface.h"
#include "ot_coap_utils.h"

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
char tx_buff[50]; 

/*UART logging enable*/
int log_in_uart = 1;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			for( int i =0; i < MSG_SIZE; i++ ){
				rx_buf[i] = NULL;
			}  
			rx_buf_pos = 0;

		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	const char *log_str = "LOG";
	const char *error_str = "ERROR";
	int print_msg = 1;

	/*If LOG or ERROR in message only print if UART_LOG=y*/
	if((strstr(buf, log_str) != NULL) || (strstr(buf, error_str) != NULL)){
		if(!log_in_uart){
			print_msg = 0;
		}	
	}
	
	if(print_msg){
		for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
		}
	}	
}

int init_uart()
{	
	/* Enable/disable UART logging*/
	#ifdef CONFIG_UART_LOG_ENABLE
		log_in_uart = 1; // Logging is enabled
	#else
		log_in_uart = 0; // Logging is disabled
	#endif

	if (!device_is_ready(uart_dev)) {
		print_uart("Device not ready \r\n");
		return 1;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			print_uart("Interrupt-driven UART API support not enabled\r\n");
		} else if (ret == -ENOSYS) {
			print_uart("UART device does not support interrupt-driven API\r\n");
		} else {
			print_uart("Error setting UART callback \r\n");
		}
		return 1;
	}
	uart_irq_rx_enable(uart_dev);
	
	return 0;
}


static void process_received_msg(char *msg_buf){
	
	// Check if wifi in message received [wifi:1]
        char* wifi_in_msg = strchr(msg_buf, 'w');
        if(*wifi_in_msg != NULL){
            const char wifi_status_char = (char)wifi_in_msg[5];
            const bool new_wifi_status = wifi_status_char == '1' ? true : false;
            set_wifi_status(new_wifi_status);
        }
        
        // Check if presence in message received [prs:1]
        char* presence_in_msg = strchr(msg_buf, 'p');
        if(*presence_in_msg != NULL){
            const char presence_status_char = (char)presence_in_msg[4];
            const bool new_presence_status = presence_status_char == '1' ? true : false;
            set_presence_status(new_presence_status);                
        }  

        // Check if electrical in message received [ele:1]
        char* ele_in_msg = strchr(msg_buf, 'e');
        if(*ele_in_msg != NULL){
            const char ele_status_char = (char)ele_in_msg[4];
            const bool new_ele_status = ele_status_char == '1' ? true : false;
            set_electrical_status(new_ele_status);
        }
}

void read_forever()
{
	print_uart("[LOG]: Reading UART for ever\r\n");

	char tx_buf[MSG_SIZE];

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
		snprintf(tx_buff, sizeof(tx_buff), "%s%s\r\n", "[LOG]: UART message received: ", tx_buf);
    	print_uart(tx_buff);

        // Check if wifi in message received
        char* wifi_in_msg = strchr(tx_buf, 'w');
        if(*wifi_in_msg != NULL){
            const char wifi_status_char = (char)wifi_in_msg[5];
            const bool new_wifi_status = wifi_status_char == '1' ? true : false;
            set_wifi_status(new_wifi_status);
        }
        
        // Check if presence in message received
        char* presence_in_msg = strchr(tx_buf, 'p');
        if(*presence_in_msg != NULL){
            const char presence_status_char = (char)presence_in_msg[4];
            const bool new_presence_status = presence_status_char == '1' ? true : false;
            set_presence_status(new_presence_status);                
        }  

        // Check if electrical in message received
        char* ele_in_msg = strchr(tx_buf, 'e');
        if(*ele_in_msg != NULL){
            const char ele_status_char = (char)ele_in_msg[4];
            const bool new_ele_status = ele_status_char == '1' ? true : false;
            set_electrical_status(new_ele_status);
        }

        // Check if oulet in message received
        char* outlet_in_msg = strchr(tx_buf, 'o');
        if(*outlet_in_msg != NULL){
            const char r1_status_char = (char)outlet_in_msg[7];
            const char r2_status_char = (char)outlet_in_msg[8];
            const char r3_status_char = (char)outlet_in_msg[9];
            const char r4_status_char = (char)outlet_in_msg[10];

            const bool new_r1_status = r1_status_char == '1' ? true : false;
            const bool new_r2_status = r2_status_char == '1' ? true : false;
            const bool new_r3_status = r3_status_char == '1' ? true : false;
            const bool new_r4_status = r4_status_char == '1' ? true : false;
            set_power_strip_status(new_r1_status, new_r2_status, new_r3_status, new_r4_status);
        }
	}
}