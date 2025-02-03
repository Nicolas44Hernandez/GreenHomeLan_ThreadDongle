
#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

void serial_cb(const struct device *dev, void *user_data);
void print_uart(char *buf);
int init_uart();
void read_forever();
static void process_received_msg(char *msg_buf);

#endif