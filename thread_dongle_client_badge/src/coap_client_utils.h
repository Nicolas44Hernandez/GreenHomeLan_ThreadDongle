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

/** @brief Send a action to the CoAP server node.
 *
 * @note The CoAP server should be paired before to have an affect.
 */
void coap_client_send_commands_to_server_message(uint8_t* msg, uint16_t msg_length);

/** @brief Request for the CoAP ressources status ressource.
 *
 */
void coap_client_send_ressources_status_request(void);

/** @brief Request for the CoAP wifi status ressource.
 *
 */
void coap_client_send_wifi_status_request(void);

/** @brief Request for post button alarm.
 *
 */
void coap_client_send_alarm(void);

/** @brief Request for the CoAP presence status ressource.
 *
 */
void coap_client_send_presence_status_request(void);

/** @brief Request for the CoAP electrical status ressource.
 *
 */
void coap_client_send_electrical_status_request(void);

/** @brief Print the orchestrator server ressources status.
 *
 */
void print_orchestrator_server_ressources(void);

/** @brief Returns the orchestrator wifi status.
 *
 */
bool get_server_wifi_status(void);

/** @brief Returns the orchestrator presence status.
 *
 */
bool get_server_presence_status(void);

/** @brief Returns the orchestrator electrical status.
 *
 */
bool get_server_electrical_status(void);




#endif

/**
 * @}
 */
