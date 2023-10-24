#include "OPCUAServer.h"

MODULE_AUTHOR("Robert Dremor");
MODULE_DESCRIPTION("Motion Controller for DCS");
MODULE_LICENSE("GPL");



static int comp_id;
extern "C" {
    int rtapi_app_main(void);
    void rtapi_app_exit(void);   
}

static OPCUAServer server;

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

OPCUAServer* globalServerInstance = nullptr;

static void globalStartFunction(void* arg1, long int arg2) {
    if (server.server) {
        UA_Server_run_iterate(server.server, true);
    }
}


int rtapi_app_main(void) {
    comp_id = hal_init("opcuaserver");
    if(comp_id < 0) {
        return comp_id;
    }

    UA_StatusCode retval = server.initialize(serverAddress, serverPort, serverCertPath, serverKeyPath);
    if (retval != UA_STATUSCODE_GOOD) {
        rtapi_print_msg(RTAPI_MSG_ERR, "OPCUA: Failed to initialize server.");
        return EXIT_FAILURE;
    }
    globalServerInstance = &server;
    hal_create_thread("opcua-thread", 1000000, 1);
    hal_export_funct("opcua", globalStartFunction, 0, 1, 0, comp_id);
   

    hal_ready(comp_id);
    return EXIT_SUCCESS;
}

void rtapi_app_exit(void) {
    hal_exit(comp_id);
    server.stop();
    
}