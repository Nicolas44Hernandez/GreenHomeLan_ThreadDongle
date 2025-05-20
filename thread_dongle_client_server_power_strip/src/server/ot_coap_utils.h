/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __OT_COAP_UTILS_H__
#define __OT_COAP_UTILS_H__

#include <thread_dongle_interface.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/openthread.h>
#include <openthread/coap.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/thread.h>

/**@brief Type definition of the function used to handle power strip status resource msg.
 */
typedef void (*power_strip_status_request_callback_t)();

int ot_coap_init(power_strip_status_request_callback_t on_power_strip_status_request);

void set_server_mode(bool server_active);

/**@brief Type definition of the function used to set power strip status resource msg.
 */
void set_power_strip_status(bool new_r1_status, bool new_r2_status, bool new_r3_status, bool new_r4_status);

/**@brief Type definition of the function used to print current power_strip status.
 */
void print_power_strip_status();

#endif
