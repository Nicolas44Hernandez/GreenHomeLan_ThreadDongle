/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __OT_COAP_UTILS_H__
#define __OT_COAP_UTILS_H__

#include <thread_dongle_interface.h>

/**@brief Type definition of the function used to handle commands resource msg.
 */
typedef void (*commands_request_callback_t)(uint8_t* msg_buf, uint8_t msg_len);
/**@brief Type definition of the function used to handle ressources status resource msg.
 */
typedef void (*ressources_status_request_callback_t)();
/**@brief Type definition of the function used to handle wifi status resource msg.
 */
typedef void (*wifi_status_request_callback_t)();
/**@brief Type definition of the function used to handle presence status resource msg.
 */
typedef void (*presence_status_request_callback_t)();

int ot_coap_init(
    ressources_status_request_callback_t on_ressources_status_request, 
    wifi_status_request_callback_t on_wifi_status_request, 
    presence_status_request_callback_t on_presence_status_request, 
    commands_request_callback_t on_commands_request
);

/**@brief Type definition of the function used to set wifi status resource msg.
 */
void set_wifi_status(bool new_status);
/**@brief Type definition of the function used to set presence status resource msg.
 */
void set_presence_status(bool new_status);

#endif
