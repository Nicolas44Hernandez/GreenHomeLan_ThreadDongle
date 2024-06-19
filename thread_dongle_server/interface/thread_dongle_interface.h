/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __COAP_SERVER_CLIENT_INTRFACE_H__
#define __COAP_SERVER_CLIENT_INTRFACE_H__

#define COAP_PORT 5683

#define MSG_BUFF_SIZE 2000
#define MSG_MAX_SIZE 20

#define COMMANDS_URI_PATH "commands"
#define RESSOURCES_URI_PATH "ressources"
#define WIFI_URI_PATH "wifi"
#define PRESENCE_URI_PATH "presence"
#define ELECTRIC_URI_PATH "energy"
#define POWER_STRIP_URI_PATH "power_strip"

#define ALARM "al_bt_em"
#define CMD1 "cmd_1"
#define KEEP_ALIVE_DEVICE_ID_1 "ka_1"
#define KEEP_ALIVE_DEVICE_ID_2 "ka_2"
#define KEEP_ALIVE_DEVICE_ID_3 "ka_3"
#define KEEP_ALIVE_DEVICE_ID_4 "ka_4"


/*LEDS configuration*/
#define RESSOURCES_STATUS_MSG_LED    0   /* RGB LED - Red */
#define COMMANDS_MSG_LED             1   /* RGB LED - White */
#define OT_CONNECTION_LED            2   /* RGB LED - Blue */


#endif
