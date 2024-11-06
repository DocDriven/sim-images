#include <open62541/common.h>
#include <open62541/nodeids.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include <sqlite3.h>
#include "tank_system.h"


UA_NodeId tankSystemTypeIdent = {1, UA_NODEIDTYPE_NUMERIC, {5000}};


UA_StatusCode defineTankSystemObjectType(UA_Server *server)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*
     * Define object type for 'tankSystem'
     */
    UA_ObjectTypeAttributes tsAttr = UA_ObjectTypeAttributes_default;
    tsAttr.displayName = UA_LOCALIZEDTEXT("en-US", "tankSystemType");

    retval = UA_Server_addObjectTypeNode(server, tankSystemTypeIdent,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "tankSystemType"), tsAttr, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'tankSystemType'. Exiting with code %u",
                    retval);
        return retval;
    }

    /*
     * Define tank system specific attributes in the custom object type
     */
    UA_VariableAttributes fillPercentageAttr = UA_VariableAttributes_default;
    fillPercentageAttr.displayName = UA_LOCALIZEDTEXT("en-US", "FillPercentage");
    fillPercentageAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    fillPercentageAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId fillPercentageIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, tankSystemTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "FillPercentage"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       fillPercentageAttr, NULL, &fillPercentageIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'FillPercentage'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, fillPercentageIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'FillPercentage'. Exiting with code %u,",
                    retval);
        return retval;
    }

    UA_VariableAttributes valvePosAttr = UA_VariableAttributes_default;
    valvePosAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ValvePosition");
    valvePosAttr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    valvePosAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId valvePosIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, tankSystemTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "ValvePosition"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       valvePosAttr, NULL, &valvePosIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'ValvePosition'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, valvePosIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'ValvePosition'. Exiting with code %u",
                    retval);
        return retval;
    }

    UA_VariableAttributes thresholdAttr = UA_VariableAttributes_default;
    thresholdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Threshold");
    thresholdAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    thresholdAttr.valueRank = UA_VALUERANK_SCALAR;
    /* thresholdAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE; */
    UA_NodeId thresholdIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, tankSystemTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Threshold"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       thresholdAttr, NULL, &thresholdIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'Threshold'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, thresholdIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'Threshold'. Exiting with code %u",
                    retval);
        return retval;
    }
    return retval;
}


UA_StatusCode addTankSystemObjectInstance(UA_Server *server, char *name, UA_NodeId *tankSystemObjectId)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                   UA_QUALIFIEDNAME(1, name),
                                   tankSystemTypeIdent, /* this refers to the object type identifier */
                                   oAttr, NULL, tankSystemObjectId);
}

