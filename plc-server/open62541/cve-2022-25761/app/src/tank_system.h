#ifndef TANK_SYSTEM_H
#define TANK_SYSTEM_H

#include <open62541/server.h>
#include <open62541/types.h>
#include <sqlite3.h>

/*
 * The object type node identifiers are made available here for user convenicence.
 * They are defined in the respective implementation files.
 */
extern UA_NodeId tankSystemTypeIdent;

/*
 * Define the data type used by tank system objects. Nodes can be instantiated only
 * after defining these types first.
 */
UA_StatusCode defineTankSystemObjectType(UA_Server *server);

/*
 * Add a single instance to the objects directory and retrieve the assigned node ID
 * that is written into 'tankSystemObjectId' for further reference
 */
UA_StatusCode addTankSystemObjectInstance(UA_Server *server, char *name, UA_NodeId *tankSystemObjectId);

#endif
