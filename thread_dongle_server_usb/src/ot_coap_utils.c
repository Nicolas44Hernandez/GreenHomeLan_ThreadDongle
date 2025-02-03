/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coap_utils.h"
#include "uart_interface.h"

uint8_t msg_buf[MSG_BUFF_SIZE];
uint16_t msg_len = 0; 
char print_buff[50]; 

struct server_context {
    struct otInstance *ot;
    ressources_status_request_callback_t on_ressources_status_request;
    wifi_status_request_callback_t on_wifi_status_request;
    presence_status_request_callback_t on_presence_status_request;
    electrical_status_request_callback_t on_eletrical_status_request;
    power_strip_status_request_callback_t on_power_strip_status_request;
    commands_request_callback_t on_commands_request;
    bool wifi_status;
    bool presence_status;
    bool electrical_status;
    bool power_strip_r1_status;
    bool power_strip_r2_status;
    bool power_strip_r3_status;
    bool power_strip_r4_status;
};

static struct server_context srv_context = {
    .ot = NULL,    
    .on_ressources_status_request = NULL,
    .on_wifi_status_request = NULL,
    .on_eletrical_status_request= NULL,
    .on_commands_request = NULL,
    .wifi_status = NULL,
    .presence_status = NULL,
    .electrical_status = NULL,
    .power_strip_r1_status = NULL,
    .power_strip_r2_status = NULL,
    .power_strip_r3_status = NULL,
    .power_strip_r4_status = NULL,
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

/**@brief Definition of CoAP resources for electrical status resources. */
static otCoapResource electrical_status_resource = {
    .mUriPath = ELECTRIC_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

/**@brief Definition of CoAP resources for power_strip status resources. */
static otCoapResource power_strip_status_resource = {
    .mUriPath = POWER_STRIP_URI_PATH,
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
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: New wifi status ", srv_context.wifi_status ? "true" : "false");
    print_uart(print_buff);
}
void set_presence_status(bool new_status){
    srv_context.presence_status = new_status;
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: New presence status ", srv_context.presence_status ? "true" : "false");
    print_uart(print_buff);
}
void set_electrical_status(bool new_status){
    srv_context.electrical_status = new_status;
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: New electrical status ", srv_context.electrical_status ? "true" : "false");
    print_uart(print_buff);
}

void set_power_strip_status(bool new_r1_status, bool new_r2_status, bool new_r3_status, bool new_r4_status){
    srv_context.power_strip_r1_status = new_r1_status;
    srv_context.power_strip_r2_status = new_r2_status;
    srv_context.power_strip_r3_status = new_r3_status;
    srv_context.power_strip_r4_status = new_r4_status;
    snprintf(print_buff, sizeof(print_buff), "%s%d%d%d%d\r\n", "[LOG]: New power strip status ", srv_context.power_strip_r1_status, srv_context.power_strip_r2_status, srv_context.power_strip_r3_status, srv_context.power_strip_r4_status );
    print_uart(print_buff);
}

void print_ressources_status(void){
    print_uart("[LOG]: Server Ressources   \r\n");
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]:     Wifi status ", srv_context.wifi_status ? "true" : "false");
    print_uart(print_buff);     
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]:     Presence status ", srv_context.presence_status ? "true" : "false");
    print_uart(print_buff);
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]:     Electrical status ", srv_context.electrical_status ? "true" : "false");
    print_uart(print_buff);
    snprintf(print_buff, sizeof(print_buff), "%s%d%d%d%d\r\n", "[LOG]:     Power strip status ", srv_context.power_strip_r1_status, srv_context.power_strip_r2_status, srv_context.power_strip_r3_status, srv_context.power_strip_r4_status );
    print_uart(print_buff);
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

    // Append electrical status to payload
    uint8_t *electrical_payload = srv_context.electrical_status ? " ele:1" : " ele:0";
    uint16_t electrical_payload_size = strlen(electrical_payload);

    // Append power strip status to payload  
    uint8_t power_strip_payload[] = " outlet:XXXX";
    uint8_t r1_status_payload = srv_context.power_strip_r1_status ? '1':'0';
    uint8_t r2_status_payload = srv_context.power_strip_r2_status ? '1':'0';
    uint8_t r3_status_payload = srv_context.power_strip_r3_status ? '1':'0';
    uint8_t r4_status_payload = srv_context.power_strip_r4_status ? '1':'0';
    power_strip_payload[8] = r1_status_payload;   
    power_strip_payload[9] = r2_status_payload;   
    power_strip_payload[10] = r3_status_payload;   
    power_strip_payload[11] = r4_status_payload;   
    uint16_t power_strip_payload_size = strlen(power_strip_payload);

    // Get payload size
    uint16_t payload_size = wifi_payload_size + presence_payload_size + electrical_payload_size + power_strip_payload_size + 1;
    uint8_t *payload = (uint8_t*) malloc(payload_size * sizeof(uint8_t));

    if(payload == NULL) {
        print_uart("[ERROR]: Error in payload memory alocation \r\n");    
        error = OT_ERROR_FAILED; 
        goto end;
    }

    // Concatenate payload
    uint16_t new_index = 0;
    for(uint16_t i = 0; i < payload_size - 1; i++){
        if(i < wifi_payload_size){   
            payload[i] = wifi_payload[i];
        }
        if((i >= wifi_payload_size) && (i < wifi_payload_size + presence_payload_size)){
            new_index = i - wifi_payload_size;
            payload[i] = presence_payload[new_index];
        }
        if((i >= wifi_payload_size + presence_payload_size) && (i < wifi_payload_size + presence_payload_size + electrical_payload_size)){
            new_index = i - wifi_payload_size - presence_payload_size;
            payload[i] = electrical_payload[new_index];
        }
        if(i >= wifi_payload_size + presence_payload_size + electrical_payload_size){   
            new_index = i - wifi_payload_size - presence_payload_size - electrical_payload_size;
            payload[i] = power_strip_payload[new_index];
        }
    }

    payload[payload_size - 1 ] = '\0';
    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: Sending response payload: ", payload);
    print_uart(print_buff);

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
    
    print_uart("[LOG]: Received ressources status request\r\n");

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

    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: Wifi response payload: ", payload);
    print_uart(print_buff);

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

    print_uart("LOG]: Received wifi status request\r\n");

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

    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: Presence response payload: ", payload);
    print_uart(print_buff);

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

    print_uart("[LOG]: Received presence status request\r\n");

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

static otError electrical_status_response_send(otMessage *request_message,
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

    // Append electrical status to payload
    uint8_t *payload = srv_context.electrical_status ? "ele:1" : "ele:0";
    uint16_t payload_size = strlen(payload) + 1;

    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: Electrical response payload: ", payload);
    print_uart(print_buff);

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

static void electrical_status_request_handler(void *context, otMessage *message,
                     const otMessageInfo *message_info)
{
    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    print_uart("[LOG]: Received electrical status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE) &&
        (otCoapMessageGetCode(message) == OT_COAP_CODE_GET)) {
        msg_info = *message_info;
        memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

        error = electrical_status_response_send(message, &msg_info);
        if (error == OT_ERROR_NONE) {
            srv_context.on_ressources_status_request();
        }
    }
}

static otError power_strip_status_response_send(otMessage *request_message,
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

    // Append power strip status to payload
    uint8_t payload[] = " outlet:XXXX";
    payload[8] = srv_context.power_strip_r1_status ? '1':'0';   
    payload[9] = srv_context.power_strip_r2_status ? '1':'0';   
    payload[10] = srv_context.power_strip_r3_status ? '1':'0';   
    payload[11] = srv_context.power_strip_r4_status ? '1':'0';   
    uint16_t payload_size = strlen(payload);    

    snprintf(print_buff, sizeof(print_buff), "%s%s\r\n", "[LOG]: Power Strip response payload: ", payload);
    print_uart(print_buff);

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

static void power_strip_status_request_handler(void *context, otMessage *message,
                     const otMessageInfo *message_info)
{
    otError error;
    otMessageInfo msg_info;

    ARG_UNUSED(context);

    print_uart("[LOG]: Received power strip status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE) &&
        (otCoapMessageGetCode(message) == OT_COAP_CODE_GET)) {
        msg_info = *message_info;
        memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));

        error = power_strip_status_response_send(message, &msg_info);
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

    print_uart("[LOG]: Commands message received\r\n");

    ARG_UNUSED(context);   

    if (otCoapMessageGetType(message) != OT_COAP_TYPE_NON_CONFIRMABLE) {
        print_uart("[ERROR]: Commands handler - Unexpected type of message\r\n");
        goto end;
    }

    if (otCoapMessageGetCode(message) != OT_COAP_CODE_PUT) {
        print_uart("[ERROR]: Commands handler - Unexpected CoAP code\r\n");
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
    print_uart("[LOG]: Uncontext msg received\r\n");
    ARG_UNUSED(context);
    ARG_UNUSED(message);
    ARG_UNUSED(message_info);
}

int ot_coap_init(
    ressources_status_request_callback_t on_ressources_status_request,
    wifi_status_request_callback_t on_wifi_status_request,
    presence_status_request_callback_t on_presence_status_request,
    electrical_status_request_callback_t on_electrical_status_request,
    power_strip_status_request_callback_t on_power_strip_status_request,
    commands_request_callback_t on_commands_request  
)
{
    otError error;

    srv_context.wifi_status = false;
    srv_context.presence_status = false;
    srv_context.electrical_status = false;

    srv_context.on_ressources_status_request = on_ressources_status_request;
    srv_context.on_wifi_status_request = on_wifi_status_request;
    srv_context.on_presence_status_request = on_presence_status_request;
    srv_context.on_eletrical_status_request = on_electrical_status_request;
    srv_context.on_power_strip_status_request = on_power_strip_status_request;
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

    electrical_status_resource.mContext = srv_context.ot;
    electrical_status_resource.mHandler = electrical_status_request_handler;

    power_strip_status_resource.mContext = srv_context.ot;
    power_strip_status_resource.mHandler = power_strip_status_request_handler;

    commands_resource.mContext = srv_context.ot;
    commands_resource.mHandler = commands_request_handler;

    otCoapSetDefaultHandler(srv_context.ot, coap_default_handler, NULL);
    otCoapAddResource(srv_context.ot, &commands_resource);
    otCoapAddResource(srv_context.ot, &ressources_status_resource);
    otCoapAddResource(srv_context.ot, &wifi_status_resource);
    otCoapAddResource(srv_context.ot, &presence_status_resource);
    otCoapAddResource(srv_context.ot, &electrical_status_resource);
    otCoapAddResource(srv_context.ot, &power_strip_status_resource);

    error = otCoapStart(srv_context.ot, COAP_PORT);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

end:
    return error == OT_ERROR_NONE ? 0 : 1;
}
