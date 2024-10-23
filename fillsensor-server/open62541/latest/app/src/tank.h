#ifndef TANK_H
#define TANK_H

#include <open62541/server.h>

/*
 * The object type node identifiers are made available here for user convenience.
 * They are defined the the respective implementation files.
 */
extern UA_NodeId waterTankTypeIdent;

/*
 * Define the data type used by water tank objects. Nodes can be instantiated only
 * after defining these types first.
 */
UA_StatusCode defineWaterTankObjectType(UA_Server *server);

/*
 * Add a single instance to the objects directory and retrieve the assigned node ID
 * that is written into 'waterTankObjectId' for further reference
 */
UA_StatusCode addWaterTankObjectInstance(UA_Server *server, char *name, UA_NodeId *waterTankObjectId);

#endif
