#ifndef RELAYS_UTILS_H
#define RELAYS_UTILS_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <thread_dongle_interface.h>

// Function declarations
void set_relays_status(bool r1_status, bool r2_status, bool r3_status, bool r4_status);
const char *get_relays_status_string(void);
int init_relays(void);

#endif 