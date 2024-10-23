#include <open62541/common.h>
#include <open62541/nodeids.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include "tank.h"


UA_NodeId waterTankTypeIdent = {1, UA_NODEIDTYPE_NUMERIC, {2000}};


UA_StatusCode defineWaterTankObjectType(UA_Server *server)
{
    UA_StatusCode retval = 0;

    /*
     * Define object type for a generic industrial device
     */
    UA_ObjectTypeAttributes eqAttr = UA_ObjectTypeAttributes_default;
    eqAttr.displayName = UA_LOCALIZEDTEXT("en-US", "EquipmentType");
    UA_NodeId equipmentTypeIdent; // gets default nodeid from server
    retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "EquipmentType"),
                                         eqAttr, NULL, &equipmentTypeIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'EquipmentType'. Exiting with code %u",
                    retval);
        return retval;
    }

    /*
     * Define generic attributes in the parent object type
     */
    UA_VariableAttributes idAttr = UA_VariableAttributes_default;
    idAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceID");
    idAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    idAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId deviceIdIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, equipmentTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "DeviceID"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       idAttr, NULL, &deviceIdIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'DeviceID'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, deviceIdIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'DeviceID'. Exiting with code %u",
                    retval);
        return retval;
    }

    UA_VariableAttributes locAttr = UA_VariableAttributes_default;
    locAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Location");
    locAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    locAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId locIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, equipmentTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Location"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       locAttr, NULL, &locIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'Location'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, locIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'Location'. Exiting with code %u",
                    retval);
        return retval;
    }
    /*
     * Specialize the industrial device type for the water tank object type
     */
    UA_ObjectTypeAttributes wtAttr = UA_ObjectTypeAttributes_default;
    wtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "waterTankType");
    retval = UA_Server_addObjectTypeNode(server, waterTankTypeIdent, equipmentTypeIdent,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "waterTankType"),
                                         wtAttr, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'waterTankType'. Exiting with code %u",
                    retval);
        return retval;
    }

    /*
     * Define water tank specific attributes in the child object type
     */
    UA_VariableAttributes capacityAttr = UA_VariableAttributes_default;
    capacityAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Capacity");
    capacityAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    capacityAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId capacityIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, waterTankTypeIdent,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Capacity"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       capacityAttr, NULL, &capacityIdent);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add node 'Capacity'. Exiting with code %u",
                    retval);
        return retval;
    }
    retval = UA_Server_addReference(server, capacityIdent,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add reference 'Capacity'. Exiting with code %u",
                    retval);
        return retval;
    }

    UA_VariableAttributes fillPercentageAttr = UA_VariableAttributes_default;
    fillPercentageAttr.displayName = UA_LOCALIZEDTEXT("en-US", "FillPercentage");
    fillPercentageAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    fillPercentageAttr.valueRank = UA_VALUERANK_SCALAR;
    fillPercentageAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId fillPercentageIdent;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, waterTankTypeIdent,
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
                    "Unable to add reference 'FillPercentage'. Exiting with code %u",
                    retval);
        return retval;
    }
    return retval;
}


UA_StatusCode addWaterTankObjectInstance(UA_Server *server, char *name, UA_NodeId *waterTankObjectId)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                   UA_QUALIFIEDNAME(1, name),
                                   waterTankTypeIdent, /* this refers to the object type identifier */
                                   oAttr, NULL, waterTankObjectId);
}
