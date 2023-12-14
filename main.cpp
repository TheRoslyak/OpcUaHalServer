#include "hal.h"
#include "hal_priv.h"
#include <rtapi_mutex.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>

static int comp_id;
static UA_Server *server = UA_Server_new();
typedef struct {
    void *valuePtr;
    hal_type_t type;
} DataSourceContext;



static UA_NodeId createFolder(const char *folderName) {
    UA_NodeId folderId; 
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)folderName);
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, (char*)folderName),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            oAttr, NULL, &folderId);
    return folderId;
}

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
        float value = *(hal_float_t *)context->valuePtr;
        UA_Variant_setScalarCopy(&dataValue->value, &value, &UA_TYPES[UA_TYPES_FLOAT]);
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
                const UA_NumericRange *range, const UA_DataValue *dataValue) {

    DataSourceContext *context = (DataSourceContext *)nodeContext;
    
    switch (context->type) {
        case HAL_FLOAT: {
            UA_Variant_hasScalarType(&dataValue->value, &UA_TYPES[UA_TYPES_FLOAT]);
            *(hal_float_t *)context->valuePtr = *(float *)dataValue->value.data;
            break;
        }
        case HAL_BIT: {
            UA_Variant_hasScalarType(&dataValue->value, &UA_TYPES[UA_TYPES_BOOLEAN]);
            *(hal_bit_t *)context->valuePtr = *(UA_Boolean *)dataValue->value.data;
            break;
        }
        case HAL_U32: {
            UA_Variant_hasScalarType(&dataValue->value, &UA_TYPES[UA_TYPES_UINT32]);
            *(hal_u32_t *)context->valuePtr = *(UA_UInt32 *)dataValue->value.data;
            break;
        }
        case HAL_S32: {
            UA_Variant_hasScalarType(&dataValue->value, &UA_TYPES[UA_TYPES_INT32]);
            *(hal_s32_t *)context->valuePtr = *(UA_Int32 *)dataValue->value.data;
            break;
        }
       
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode addVariableWithDataSource(const char *variableName, hal_type_t type, void *valuePtr,UA_NodeId folder) {
    
    DataSourceContext *dsContext = new DataSourceContext;
    dsContext->valuePtr = valuePtr;
    dsContext->type = type;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)variableName);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myVariableNodeId = UA_NODEID_STRING(1, (char*)variableName);
    UA_QualifiedName myVariableName = UA_QUALIFIEDNAME(1, (char*)variableName);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    
    UA_DataSource dataSource;
    dataSource.read = readDataSource;
    dataSource.write = writeDataSource;

    
    UA_StatusCode retVal = UA_Server_addDataSourceVariableNode(server, myVariableNodeId, folder,
                                                               parentReferenceNodeId, myVariableName,
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr,
                                                               dataSource, dsContext, NULL);
    return retVal;
}

int main(int argc, char *argv[]) {
    comp_id = hal_init("opcuaserver");
    if (comp_id < 0) return -1;

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    rtapi_mutex_get(&(hal_data->mutex));
    
    UA_NodeId pinsFolderId = createFolder("Pins");
    hal_pin_t *currentPin;
          for (currentPin = SHMPTR(hal_data->pin_list_ptr); 
               currentPin; 
               currentPin = SHMPTR(currentPin->next_ptr)) {
        addVariableWithDataSource(currentPin->name,
                                  currentPin->type, 
                                  currentPin->signal ? static_cast<void*>(SHMPTR(SHMPTR(currentPin->signal)->data_ptr)) 
                                                     : static_cast<void*>(&(currentPin->dummysig)),
                                  pinsFolderId);
    }

    UA_NodeId signalsFolderId = createFolder("Signals");
    hal_sig_t *currentSig;
          for (currentSig = SHMPTR(hal_data->sig_list_ptr);
               currentSig; currentSig = SHMPTR(currentSig->next_ptr)) {
    addVariableWithDataSource(currentSig->name, 
                              currentSig->type, 
                       SHMPTR(currentSig->data_ptr),
                              signalsFolderId);
    }

    UA_NodeId parametersFolderId = createFolder("Parameters");
    hal_param_t *currentParam;
            for (currentParam = SHMPTR(hal_data->param_list_ptr); 
                 currentParam;
                 currentParam = SHMPTR(currentParam->next_ptr)) {
     addVariableWithDataSource(currentParam->name,
                               currentParam->type,
                        SHMPTR(currentParam->data_ptr),
                               parametersFolderId);
    }
    
    rtapi_mutex_give(&hal_data->mutex);

    hal_ready(comp_id);
    UA_ServerConfig *config = UA_Server_getConfig(server);
config->publishingIntervalLimits.min = 1.0; // минимальный интервал в мс
config->publishingIntervalLimits.max = 1.0; // максимальный интервал в мс
config->samplingIntervalLimits.min = 1.0; // минимальный интервал опроса в мс
config->samplingIntervalLimits.max = 1.0; // максимальный интервал опроса в мс

    UA_Server_run(server, &running);
    
    hal_exit(comp_id);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    
    return EXIT_SUCCESS;
}
