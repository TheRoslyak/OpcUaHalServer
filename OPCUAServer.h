#ifndef OPCUASERVER_H
#define OPCUASERVER_H

#include <cstdio>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <signal.h>
//#include <rtapi.h>
//#include <rtapi_app.h>
#include <hal.h>
#include <hal_priv.h>
//#include <rtapi_mutex.h>

class OPCUAServer {
private:
    UA_NodeId variableId;
    //void processPin(hal_pin_t *pin);
    //void processSig(hal_sig_t *sig);
    //void processParam(hal_param_t *param);

public:
    UA_Server *server;
    
    OPCUAServer();
    ~OPCUAServer();

    UA_StatusCode  initialize(const char* address, int port, const char* certPath, const char* keyPath);
    void stop();
    
    UA_StatusCode startInstance();
    //void initializeHALData();
    //void runIterate();
    
};

#endif // OPCUASERVER_H
