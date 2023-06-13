#pragma once

#define OPC_USER "hyrel"
#define OPC_PASSWORD "hyrel"
#define OPC_PORT    4840

#define BASE_IP_EVENT WIFI_EVENT
#define GOT_IP_EVENT IP_EVENT_STA_GOT_IP
#define DISCONNECT_EVENT WIFI_EVENT_STA_DISCONNECTED
#define EXAMPLE_INTERFACE TCPIP_ADAPTER_IF_STA

void opcua_task(void* arg);
void opc_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);

void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);