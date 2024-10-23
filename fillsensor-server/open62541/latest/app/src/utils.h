#ifndef UTILS_H
#define UTILS_H

#include <open62541/server.h>


/*
 * Retrieve the attribute node ID from the hierarchy below
 * startNodeIdent
 */
UA_StatusCode findAttributeNodeId(
    UA_Server *server,
    const UA_NodeId *startNodeIdent,
    const UA_QualifiedName *qName,
    UA_NodeId *attributeNodeId);

#endif
