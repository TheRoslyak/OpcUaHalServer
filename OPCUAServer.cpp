#include "OPCUAServer.h"

OPCUAServer::OPCUAServer() : server(nullptr) {}

OPCUAServer::~OPCUAServer() {
    stop();
}


UA_StatusCode OPCUAServer::initialize(const char* address, int port, const char* certPath, const char* keyPath) {
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

void OPCUAServer::stop() {
    if (server) {
        // Сначала остановите сервер.
        UA_Server_run_shutdown(server);
        //running = false;
        
        // После этого безопасно удалите его.
        UA_Server_delete(server);
        server = nullptr;
        rtapi_print_msg(RTAPI_MSG_INFO, "OPCUA: Server stopped and deleted.");
    }
}
