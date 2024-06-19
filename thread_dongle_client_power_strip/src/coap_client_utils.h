/**
 * @file
 * @defgroup coap_client_utils API for coap_client_* samples
 * @{
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __COAP_CLIENT_UTILS_H__
#define __COAP_CLIENT_UTILS_H__

/** @brief Type indicates function called when OpenThread connection
 *         is established.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ot_connection_cb_t)(struct k_work *item);

/** @brief Type indicates function called when OpenThread connection is ended.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ot_disconnection_cb_t)(struct k_work *item);

/** @brief Initialize CoAP client utilities.
 */
void coap_client_utils_init(ot_connection_cb_t on_connect,
                   ot_disconnection_cb_t on_disconnect);

/** @brief Request for post keep alive msg.
 *
 */
void coap_client_send_keep_alive(void);

/** @brief Request for the CoAP power strip status ressource.
 *
 */
void coap_client_send_power_strip_status_request(void);

/** @brief Returns the orchestrator r1 status.
 *
 */
bool get_server_r1_status(void);

/** @brief Returns the orchestrator r2 status.
 *
 */
bool get_server_r2_status(void);

/** @brief Returns the orchestrator r3 status.
 *
 */
bool get_server_r3_status(void);

/** @brief Returns the orchestrator r4 status.
 *
 */
bool get_server_r4_status(void);

#endif

/**
 * @}
 */
