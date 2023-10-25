//#include "OPCUAServer.h"
#include <open62541/server.h>
#include <open62541/server_config_default.h>
//#include <open62541/plugin/log_stdout.h>
#include <hal.h>
#include <hal_priv.h>

MODULE_AUTHOR("Robert Dremor");
MODULE_DESCRIPTION("Motion Controller for DCS");
MODULE_LICENSE("GPL");



static int comp_id;
/*extern "C" {
    int rtapi_app_main(void);
    void rtapi_app_exit(void);   
}*/

static UA_Server *server = NULL;

// Адрес сервера (например, "localhost" или "192.168.1.1")
static char *serverAddress = "localhost"; 
RTAPI_MP_STRING(serverAddress, "OPC UA Server address");

// Порт сервера (например, 4840 - стандартный порт для OPC UA)
static int serverPort = 4840; 
RTAPI_MP_INT(serverPort, "OPC UA Server port");

// Путь к сертификату сервера (если используется)
static char *serverCertPath = NULL;
RTAPI_MP_STRING(serverCertPath, "Path to OPC UA Server certificate");

// Путь к приватному ключу сервера (если используется)
static char *serverKeyPath = NULL;
RTAPI_MP_STRING(serverKeyPath, "Path to OPC UA Server private key");

static void globalStartFunction() {
    if (server) {
        UA_Server_run_iterate(server, true);
    }
}

UA_StatusCode initialize_server() {
    server = UA_Server_new();
    if (!server) {
        rtapi_print_msg(RTAPI_MSG_ERR, "OPCUA: Failed to create new server instance.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    // Add a sample variable
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    UA_Server_run_startup(server);
    return UA_STATUSCODE_GOOD;
}




int rtapi_app_main(void) {
    comp_id = hal_init("opcuaserver");
    if(comp_id < 0) {
        return comp_id;
    }

    initialize_server();

    hal_create_thread("opcua-thread", 400000000, 0);
    hal_export_funct("opcua", globalStartFunction, 0,0,0, comp_id);

    hal_ready(comp_id);
    return EXIT_SUCCESS;
}

void rtapi_app_exit(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    hal_exit(comp_id);
}