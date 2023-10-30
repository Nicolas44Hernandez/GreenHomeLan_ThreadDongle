/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __COAP_SERVER_CLIENT_INTRFACE_H__
#define __COAP_SERVER_CLIENT_INTRFACE_H__

#define COAP_PORT 5683

#define MSG_BUFF_SIZE 20
#define MSG_MAX_SIZE 10

#define COMMANDS_URI_PATH "commands"
#define RESSOURCES_URI_PATH "ressources"
#define WIFI_URI_PATH "wifi"
#define PRESENCE_URI_PATH "presence"


/*LEDS configuration*/
#define RESSOURCES_STATUS_MSG_LED    0   /* RGB LED - Red */
#define COMMANDS_MSG_LED             1   /* RGB LED - White */
#define OT_CONNECTION_LED            2   /* RGB LED - Blue */


#endif
