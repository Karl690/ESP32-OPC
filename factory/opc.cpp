#include "Arduino.h"
#include <pthread.h>
#include "WiFi.h"
#include "open62541.h"
#include "opc.h"
#include "nvs_flash.h"
#include "nvs.h"


bool g_bRunning  = false;
UA_NodeId   g_lastNodeId;
static UA_Boolean running = true;

static esp_netif_t *s_example_esp_netif = NULL;
static UA_Boolean isServerCreated = false;


static UA_UsernamePasswordLogin logins[3] = {
    {UA_STRING_STATIC("karl690"), UA_STRING_STATIC("karl690")},
    {UA_STRING_STATIC("hyrel"), UA_STRING_STATIC("hyrel")}
};

static UA_StatusCode
UA_ServerConfig_setUriName(UA_ServerConfig *uaServerConfig, const char *uri, const char *name)
{
    // delete pre-initialized values
    UA_String_clear(&uaServerConfig->applicationDescription.applicationUri);
    UA_LocalizedText_clear(&uaServerConfig->applicationDescription.applicationName);

    uaServerConfig->applicationDescription.applicationUri = UA_String_fromChars(uri);
    uaServerConfig->applicationDescription.applicationName.locale = UA_STRING_NULL;
    uaServerConfig->applicationDescription.applicationName.text = UA_String_fromChars(name);

    for (size_t i = 0; i < uaServerConfig->endpointsSize; i++)
    {
        UA_String_clear(&uaServerConfig->endpoints[i].server.applicationUri);
        UA_LocalizedText_clear(
            &uaServerConfig->endpoints[i].server.applicationName);

        UA_String_copy(&uaServerConfig->applicationDescription.applicationUri,
                       &uaServerConfig->endpoints[i].server.applicationUri);

        UA_LocalizedText_copy(&uaServerConfig->applicationDescription.applicationName,
                              &uaServerConfig->endpoints[i].server.applicationName);
    }

    return UA_STATUSCODE_GOOD;
}

void
addRelay0ControlNode(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Relay0");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "Control Relay number 0.");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "Control Relay number 0.");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource relay0;
    //Configure GPIOs just before adding relay 0 - not a good practice.
    //configureGPIO();
    //relay0.read = readRelay0State;
    //relay0.write = setRelay0State;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        relay0, NULL, NULL);
}


void opc_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    // if (sntp_initialized != true)
    // {
    //     if (timeinfo.tm_year < (2016 - 1900))
    //     {
    //         ESP_LOGI(SNTP_TAG, "Time is not set yet. Settting up network connection and getting time over NTP.");
    //         if (!obtain_time())
    //         {
    //             ESP_LOGE(SNTP_TAG, "Could not get time from NTP. Using default timestamp.");
    //         }
    //         time(&now);
    //     }
    //     localtime_r(&now, &timeinfo);
    //     ESP_LOGI(SNTP_TAG, "Current time: %d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    // }
    Serial.print("------------------WIFI STARTED-------------");
    if (!isServerCreated)
    {
        //xTaskCreatePinnedToCore(opcua_task, "opcua_task", 24336, NULL, 10, NULL, 1);
        // ESP_LOGI(MEMORY_TAG, "Heap size after OPC UA Task : %d", esp_get_free_heap_size());
        opcua_task(NULL);
        isServerCreated = true;
    }
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    Serial.print("------------------WIFI DISCONNECTED-------------");
}



void thread_opc_task(void* arg) {
    UA_Int32 sendBufferSize = 16384;
    UA_Int32 recvBufferSize = 16384;    
    
    ESP_LOGI(TAG, "Fire up OPC UA Server.");
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimalCustomBuffer(config, 4840, 0, sendBufferSize, recvBufferSize);

    const char *appUri = "open62541.esp321.server";
    UA_String hostName = UA_STRING("opcua-esp32");

    UA_ServerConfig_setUriName(config, appUri, "OPC_UA_Server_ESP32");
    UA_ServerConfig_setCustomHostname(config, hostName);

    config->accessControl.clear(&config->accessControl);
    config->shutdownDelay = 5000.0;
    logins[1].username = UA_STRING("hyrel");
    logins[1].password = UA_STRING("hyrel");
    UA_StatusCode retval = UA_AccessControl_default(config, false,
        &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 2, logins);

    Serial.printf("Heap Left : %d", xPortGetFreeHeapSize());
    retval = UA_Server_run_startup(server);
    g_bRunning = true;
    if (retval == UA_STATUSCODE_GOOD)
    {
        
        Serial.printf("-----------------OPC Start -----------------\n");
        while (g_bRunning) {
            UA_Server_run_iterate(server, false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    if (!g_bRunning)
        UA_Server_run_shutdown(server);
    g_bRunning = false;
}
void *ThreadRunOpcFunc(void *threadid) {
    xTaskCreatePinnedToCore(thread_opc_task, "opcua_task", 24336, NULL, 10, NULL, 1);
    return 0;
}

void opcua_task(void* arg) {
    // pthread_t thread_id;
    // pthread_create(&thread_id, NULL, &ThreadRunOpcFunc, NULL);

    xTaskCreatePinnedToCore(thread_opc_task, "opcua_task", 24336, NULL, 10, NULL, 1);
}


