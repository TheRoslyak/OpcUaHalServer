#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include "hal.h"
#include "hal_priv.h"
#include <rtapi_mutex.h>
#include <signal.h>
#include <stdio.h> 
static int comp_id;
static UA_Server *server = NULL;
typedef struct {
    void *valuePtr;
    hal_type_t type;
} DataSourceContext;

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}


static UA_StatusCode 
readDataSource(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
               UA_DataValue *dataValue) {
    

    DataSourceContext *context = (DataSourceContext *)nodeContext;
    

    switch (context->type) {
    case HAL_FLOAT: {
        double value = *(hal_float_t *)context->valuePtr;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    }
        case HAL_BIT: {
        bool value = (bool)*(hal_bit_t *)context->valuePtr;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    }
        case HAL_U32: {
        unsigned int value = *(hal_u32_t *)context->valuePtr;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    }
        case HAL_S32: {
        int value = *(hal_s32_t *)context->valuePtr;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_INT32]);
        break;
    }

    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    dataValue->hasValue = true;
    
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
writeDataSource(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                const UA_NumericRange *range, const UA_DataValue *data) {
    
    DataSourceContext *dsContext = (DataSourceContext *)nodeContext;
    void *valuePtr = dsContext->valuePtr;

    switch (dsContext->type) { 
    case HAL_FLOAT: {
        hal_float_t *target = (hal_float_t *)valuePtr;
        *target = *(double *)data->value.data;
        break;
    }
    case HAL_BIT: {
        hal_bit_t *target = (hal_bit_t *)valuePtr;
        *target = *(bool *)data->value.data;
        break;
    }
    case HAL_U32: {
        hal_u32_t *target = (hal_u32_t *)valuePtr;
        *target = *(unsigned int *)data->value.data;
        break;
    }
    case HAL_S32: {
        hal_s32_t *target = (hal_s32_t *)valuePtr;
        *target = *(int *)data->value.data;
        break;
    }
    default:
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode addVariableWithDataSource(const char *variableName, hal_type_t type, void *valuePtr) {
    
    DataSourceContext *dsContext = new DataSourceContext;
    dsContext->valuePtr = valuePtr;
    dsContext->type = type;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
    
    UA_NodeId myVariableNodeId = UA_NODEID_STRING(1, (char*)variableName);
    UA_QualifiedName myVariableName = UA_QUALIFIEDNAME(1, (char*)variableName);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    
    UA_DataSource ds;
    ds.read = readDataSource;
    ds.write = writeDataSource;

    
    UA_StatusCode retVal = UA_Server_addDataSourceVariableNode(server, myVariableNodeId, parentNodeId,
                                                               parentReferenceNodeId, myVariableName,
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr,
                                                               ds, dsContext, NULL);
    return retVal;
}
void processPin(hal_pin_t *pin) {
    void *valuePtr = NULL;
    if (pin->signal == 0) {   
        valuePtr = &(pin->dummysig);
    } else {
        hal_sig_t *sig = SHMPTR(pin->signal);
        valuePtr = SHMPTR(sig->data_ptr);
    }
    const char *name =  (pin->name); 
    hal_type_t type  =   pin->type; 
    addVariableWithDataSource(name, type, valuePtr);
}

void processSig(hal_sig_t *sig) {
    void *data_ptr = SHMPTR(sig->data_ptr);
    const char *name =      sig->name;
    hal_type_t type =       sig->type;
    addVariableWithDataSource(name, type, data_ptr);

}
void processParam(hal_param_t *param) {
    void *data_ptr = SHMPTR(param->data_ptr);
    const char *name =      param->name;
    hal_type_t type =       param->type;
    addVariableWithDataSource(name, type, data_ptr);
}

UA_StatusCode initialize_server() {

    comp_id = hal_init("opcuaserver");
    if (comp_id < 0) return -1;
    
    server = UA_Server_new();
    if (!server) {
        rtapi_print_msg(RTAPI_MSG_ERR, "OPCUA: Failed to create new server instance.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    rtapi_mutex_get(&(hal_data->mutex));
    

    hal_pin_t *currentPin;
          for (currentPin = SHMPTR(hal_data->pin_list_ptr); 
               currentPin; 
               currentPin = SHMPTR(currentPin->next_ptr)) {
    processPin(currentPin);
    }

    
    hal_sig_t *currentSig;
          for (currentSig = SHMPTR(hal_data->sig_list_ptr);
               currentSig; currentSig = SHMPTR(currentSig->next_ptr)) {
    processSig(currentSig);
    }

    
    hal_param_t *currentParam;
            for (currentParam = SHMPTR(hal_data->param_list_ptr); 
                 currentParam;
                 currentParam = SHMPTR(currentParam->next_ptr)) {
    processParam(currentParam);
    }
    
    rtapi_mutex_give(&hal_data->mutex);
    
    return UA_STATUSCODE_GOOD;
}




int main(int argc, char *argv[]) {
    
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);


    initialize_server();

    hal_ready(comp_id);
    UA_Server_run(server, &running);
    return EXIT_SUCCESS;
}

void cleanup() {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    hal_exit(comp_id);
}