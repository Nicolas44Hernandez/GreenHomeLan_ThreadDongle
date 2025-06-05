/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_coap_utils.h"
#include "relays/relays_utils.h"


struct server_context {
    struct otInstance *ot;
    power_strip_status_request_callback_t on_power_strip_status_request;
};

static struct server_context srv_context = {
    .ot = NULL,
};

/**@brief Definition of CoAP resources for power_strip status resources. */
static otCoapResource power_strip_status_resource = {
    .mUriPath = POWER_STRIP_BACKUP_URI_PATH,
    .mHandler = NULL,
    .mContext = NULL,
    .mNext = NULL,
};

bool server_running = false;

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

    // Get relays status
    const char *relays_status = get_relays_status_string();

    // Append to payload
    char payload[16];
    snprintf(payload, sizeof(payload), " outlet:%s", relays_status);     
    uint16_t payload_size = strlen(payload);    

    printk("THREAD SERVER [INFO]: Power Strip response payload: %s \r\n", payload);

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
    if (!server_running){
        return;
    }
    otError error;
    otMessageInfo msg_info;
    uint8_t coap_code;

    ARG_UNUSED(context);

    printk("THREAD SERVER [INFO]: Received power strip status request\r\n");

    if ((otCoapMessageGetType(message) == OT_COAP_TYPE_NON_CONFIRMABLE)) {
        coap_code = otCoapMessageGetCode(message);

        if (coap_code == OT_COAP_CODE_GET) {

            printk("THREAD SERVER [INFO]: GET Received\r\n");

            // Handle GET request
            msg_info = *message_info;
            memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));
            error = power_strip_status_response_send(message, &msg_info);
            if (error == OT_ERROR_NONE) {
                srv_context.on_power_strip_status_request();
            }
        } else if (coap_code == OT_COAP_CODE_PUT) {
            
            // Handle PUT request

            printk("THREAD SERVER [INFO]: PUT Received\r\n");            
            
            // Read payload
            uint8_t payload[128];
            uint16_t payload_len = otMessageGetLength(message) - otMessageGetOffset(message);
            otMessageRead(message, otMessageGetOffset(message), payload, payload_len);

            // Process payload
            printk("PAYLOAD RECEIVED: %s\n", payload);  
            // Check if outlet in payload
            char* outlet_in_payload = strchr(payload, 'o');
            //outlet:1111
            if(*outlet_in_payload != NULL){
                // Retreive relays status
                const char r1_status_char = (char)payload[8];
                const char r2_status_char = (char)payload[9];
                const char r3_status_char = (char)payload[10];
                const char r4_status_char = (char)payload[11];
                bool r1_received_status = r1_status_char == '1' ? true : false;   
                bool r2_received_status = r2_status_char == '1' ? true : false;   
                bool r3_received_status = r3_status_char == '1' ? true : false;   
                bool r4_received_status = r4_status_char == '1' ? true : false; 

                // Update relays status 
                set_relays_status(r1_received_status, r2_received_status, r3_received_status, r4_received_status);       
            }

            // Send response
            msg_info = *message_info;
            memset(&msg_info.mSockAddr, 0, sizeof(msg_info.mSockAddr));
            error = power_strip_status_response_send(message, &msg_info);
            if (error == OT_ERROR_NONE) {
                srv_context.on_power_strip_status_request();
            }
        }
    }
}

static void coap_default_handler(void *context, otMessage *message,
                 const otMessageInfo *message_info)
{
    printk("THREAD SERVER [INFO]: Uncontext msg received\r\n");
    ARG_UNUSED(context);
    ARG_UNUSED(message);
    ARG_UNUSED(message_info);
}

int ot_coap_init(power_strip_status_request_callback_t on_power_strip_status_request)
{
    otError error;

    srv_context.on_power_strip_status_request = on_power_strip_status_request;

    srv_context.ot = openthread_get_default_instance();
    if (!srv_context.ot) {
        error = OT_ERROR_FAILED;
        goto end;
    }

    power_strip_status_resource.mContext = srv_context.ot;
    power_strip_status_resource.mHandler = power_strip_status_request_handler;

    otCoapSetDefaultHandler(srv_context.ot, coap_default_handler, NULL);
    otCoapAddResource(srv_context.ot, &power_strip_status_resource);

    error = otCoapStart(srv_context.ot, COAP_PORT);
    if (error != OT_ERROR_NONE) {
        goto end;
    }

end:
    return error == OT_ERROR_NONE ? 0 : 1;
}

void set_server_mode(bool server_active){
    server_running=server_active;
}