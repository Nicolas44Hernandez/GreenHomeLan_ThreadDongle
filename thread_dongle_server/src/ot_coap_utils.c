/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/openthread.h>
#include <openthread/coap.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/thread.h>
#include "ot_coap_utils.h"

uint8_t msg_buf[MSG_BUFF_SIZE];
uint16_t msg_len = 0; 

struct server_context {
    struct otInstance *ot;
    ressources_status_request_callback_t on_ressources_status_request;
    wifi_status_request_callback_t on_wifi_status_request;
    presence_status_request_callback_t on_presence_status_request;
    commands_request_callback_t on_commands_request;
    bool wifi_status;
    bool presence_status;
};

static struct server_context srv_context = {
    .ot = NULL,    
    .on_ressources_status_request = NULL,
    .on_wifi_status_request = NULL,
    .on_commands_request = NULL,
    .wifi_status = NULL,
    .presence_status = NULL,
};

/**@brief Definition of CoAP resources for orchestrator status resources. */
static otCoapResource ressources_status_resource = {
    .mUriPath = RESSOURCES_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

/**@brief Definition of CoAP resources for wifi status resources. */
static otCoapResource wifi_status_resource = {
    .mUriPath = WIFI_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

/**@brief Definition of CoAP resources for presence status resources. */
static otCoapResource presence_status_resource = {
    .mUriPath = PRESENCE_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

/**@brief Definition of CoAP resources for commands. */
static otCoapResource commands_resource = {
    .mUriPath = COMMANDS_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

void set_wifi_status(bool new_status){
    srv_context.wifi_status = new_status;
    printk("SERVER [DEBBUG]: New wifi status: %s \n\r", srv_context.wifi_status ? "true" : "false");
}
void set_presence_status(bool new_status){
    srv_context.presence_status = new_status;
    printk("SERVER [DEBBUG]: New presence status: %s \n\r", srv_context.presence_status ? "true" : "false");
}

static otError ressources_status_response_send(otMessage *request_message,
                      const otMessageInfo *message_info)
{
    otError error = OT_ERROR_NO_BUFS;
    otMessage *response;    

    response = otCoapNewMessage(srv_context.ot, NULL);
    if (response == NULL) {
        goto end;
    }

    otCoapMessageInit(response, OT_COAP_TYPE_NON_CONFIRMABLE,
              OT_COAP_CODE_CONTENT);

    error = otCoapMessageSetToken(
        response, otCoapMessageGetToken(request_message),
        otCoapMessageGetTokenLength(request_message));
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapMessageSetPayloadMarker(response);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    // Append wifi status to payload
    uint8_t *wifi_payload = srv_context.wifi_status ? "wifi:1" : "wifi:0";
    uint16_t wifi_payload_size = strlen(wifi_payload);
    
    // Append ressources status to payload
    uint8_t *presence_payload = srv_context.presence_status ? " prs:1" : " prs:0";
    uint16_t presence_payload_size = strlen(presence_payload);

    // Get payload size
    uint16_t payload_size = wifi_payload_size + presence_payload_size + 1;
    uint8_t *payload = (uint8_t*) malloc(payload_size * sizeof(uint8_t));

    if(payload == NULL) {
        printk("THREAD [ERROR]: Error in payload memory alocation");
        error = OT_ERROR_FAILED; 
        goto end;
    }

    // Concatenate presence payload
    for(int i = 0; i < payload_size - 1; i++){
        if(i < wifi_payload_size){
            payload[i] = wifi_payload[i];
        }
        else{
            payload[i] = presence_payload[i - wifi_payload_size];
        }
        
    }
    payload[payload_size - 1 ] = '\0';
    printk("THREAD [DEBBUG]: Sending response payload: %s  payload_size: %d \n\r", payload, payload_size);

    error = otMessageAppend(response, payload, payload_size);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapSendResponse(srv_context.ot, response, message_info);

end:
    if (error != OT_ERROR_NONE && response != NULL) {
        otMessageFree(response);
    }

    free(payload);
    return error;
}

static void ressources_status_request_handler(void *context, otMessage *message,
                     const otMessageInfo *message_info)
{
    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    printk("THREAD [DEBBUG]: Received ressources status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE) &&
        (otCoapMessageGetCode(message) == OT_COAP_CODE_GET)) {
        msg_info = *message_info;
        memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

        error = ressources_status_response_send(message, &msg_info);
        if (error == OT_ERROR_NONE) {
            srv_context.on_ressources_status_request();
        }
    }
}

static otError wifi_status_response_send(otMessage *request_message,
                      const otMessageInfo *message_info)
{
    otError error = OT_ERROR_NO_BUFS;
    otMessage *response;    

    response = otCoapNewMessage(srv_context.ot, NULL);
    if (response == NULL) {
        goto end;
    }

    otCoapMessageInit(response, OT_COAP_TYPE_NON_CONFIRMABLE,
              OT_COAP_CODE_CONTENT);

    error = otCoapMessageSetToken(
        response, otCoapMessageGetToken(request_message),
        otCoapMessageGetTokenLength(request_message));
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapMessageSetPayloadMarker(response);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    // Append wifi status to payload
    uint8_t *payload = srv_context.wifi_status ? "wifi:1" : "wifi:0";
    uint16_t payload_size = strlen(payload) + 1;

    printk("THREAD [DEBBUG]: Wifi response payload: %s  payload_size: %d \n\r", payload, payload_size);

    error = otMessageAppend(response, payload, payload_size);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapSendResponse(srv_context.ot, response, message_info);

end:
    if (error != OT_ERROR_NONE && response != NULL) {
        otMessageFree(response);
    }

    return error;
}

static void wifi_status_request_handler(void *context, otMessage *message,
                     const otMessageInfo *message_info)
{
    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    printk("THREAD [DEBBUG]: Received wifi status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE) &&
        (otCoapMessageGetCode(message) == OT_COAP_CODE_GET)) {
        msg_info = *message_info;
        memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

        error = wifi_status_response_send(message, &msg_info);
        if (error == OT_ERROR_NONE) {
            srv_context.on_ressources_status_request();
        }
    }
}

static otError presence_status_response_send(otMessage *request_message,
                      const otMessageInfo *message_info)
{
    otError error = OT_ERROR_NO_BUFS;
    otMessage *response;    

    response = otCoapNewMessage(srv_context.ot, NULL);
    if (response == NULL) {
        goto end;
    }

    otCoapMessageInit(response, OT_COAP_TYPE_NON_CONFIRMABLE,
              OT_COAP_CODE_CONTENT);

    error = otCoapMessageSetToken(
        response, otCoapMessageGetToken(request_message),
        otCoapMessageGetTokenLength(request_message));
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapMessageSetPayloadMarker(response);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    // Append presence status to payload
    uint8_t *payload = srv_context.presence_status ? "prs:1" : "prs:0";
    uint16_t payload_size = strlen(payload) + 1;

    printk("THREAD [DEBBUG]: Presence response payload: %s  payload_size: %d \n\r", payload, payload_size);

    error = otMessageAppend(response, payload, payload_size);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapSendResponse(srv_context.ot, response, message_info);

end:
    if (error != OT_ERROR_NONE && response != NULL) {
        otMessageFree(response);
    }

    return error;
}

static void presence_status_request_handler(void *context, otMessage *message,
                     const otMessageInfo *message_info)
{
    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    printk("THREAD [DEBBUG]: Received presence status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE) &&
        (otCoapMessageGetCode(message) == OT_COAP_CODE_GET)) {
        msg_info = *message_info;
        memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

        error = presence_status_response_send(message, &msg_info);
        if (error == OT_ERROR_NONE) {
            srv_context.on_ressources_status_request();
        }
    }
}

static otError commands_msg_response_send(otMessage *request_message,
                      const otMessageInfo *message_info)
{
    otError error = OT_ERROR_NO_BUFS;
    otMessage *response;    

    response = otCoapNewMessage(srv_context.ot, NULL);
    if (response == NULL) {
        goto end;
    }

    otCoapMessageInit(response, OT_COAP_TYPE_NON_CONFIRMABLE,
              OT_COAP_CODE_CONTENT);

    error = otCoapMessageSetToken(
        response, otCoapMessageGetToken(request_message),
        otCoapMessageGetTokenLength(request_message));
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapMessageSetPayloadMarker(response);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    // Append presence status to payload
    uint8_t *payload = "CMD:OK";
    uint16_t payload_size = strlen(payload) + 1;

    error = otMessageAppend(response, payload, payload_size);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

    error = otCoapSendResponse(srv_context.ot, response, message_info);

end:
    if (error != OT_ERROR_NONE && response != NULL) {
        otMessageFree(response);
    }

    return error;
}

static void commands_request_handler(void *context, otMessage *message,
                  const otMessageInfo *message_info)
{

    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    printk("THREAD [DEBBUG]: Commands message received\r\n");
    ARG_UNUSED(context);   

    if (otCoapMessageGetType(message) != OT_COAP_TYPE_NON_CONFIRMABLE) {
        printk("THREAD [ERROR]: Commands handler - Unexpected type of message");
        goto end;
    }

    if (otCoapMessageGetCode(message) != OT_COAP_CODE_PUT) {
        printk("THREAD [ERROR]: Commands handler - Unexpected CoAP code");
        goto end;
    }

    msg_info = *message_info;
    memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

    error = commands_msg_response_send(message, &msg_info);
    if (error == OT_ERROR_NONE) {
        msg_len = otMessageRead(message, otMessageGetOffset(message), msg_buf, MSG_BUFF_SIZE);
        srv_context.on_commands_request(&msg_buf, msg_len);
    }   
end:
    return;
}

static void coap_default_handler(void *context, otMessage *message,
                 const otMessageInfo *message_info)
{
    printk("THREAD [DEBBUG]: Uncontext msg received\r\n");
    ARG_UNUSED(context);
    ARG_UNUSED(message);
    ARG_UNUSED(message_info);
}

int ot_coap_init(
    ressources_status_request_callback_t on_ressources_status_request,
    wifi_status_request_callback_t on_wifi_status_request,
    presence_status_request_callback_t on_presence_status_request,
    commands_request_callback_t on_commands_request  
)
{
    otError error;

    srv_context.wifi_status = false;
    srv_context.presence_status = false;

    srv_context.on_ressources_status_request = on_ressources_status_request;
    srv_context.on_wifi_status_request = on_wifi_status_request;
    srv_context.on_presence_status_request = on_presence_status_request;
    srv_context.on_commands_request = on_commands_request;    

    srv_context.ot = openthread_get_default_instance();
    if (!srv_context.ot) {
        error = OT_ERROR_FAILED;
        goto end;
    }

    ressources_status_resource.mContext = srv_context.ot;
    ressources_status_resource.mHandler = ressources_status_request_handler;

    wifi_status_resource.mContext = srv_context.ot;
    wifi_status_resource.mHandler = wifi_status_request_handler;

    presence_status_resource.mContext = srv_context.ot;
    presence_status_resource.mHandler = presence_status_request_handler;

    commands_resource.mContext = srv_context.ot;
    commands_resource.mHandler = commands_request_handler;

    otCoapSetDefaultHandler(srv_context.ot, coap_default_handler, NULL);
    otCoapAddResource(srv_context.ot, &commands_resource);
    otCoapAddResource(srv_context.ot, &ressources_status_resource);
    otCoapAddResource(srv_context.ot, &wifi_status_resource);
    otCoapAddResource(srv_context.ot, &presence_status_resource);

    error = otCoapStart(srv_context.ot, COAP_PORT);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

end:
    return error == OT_ERROR_NONE ? 0 : 1;
}
