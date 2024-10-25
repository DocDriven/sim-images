#ifndef UTILS_H
#define UTILS_H

#include <open62541/types.h>


/*
 * Parses the certificate and key file. Copied from example code
 * found on the open62541 github repository
 */
UA_ByteString loadFile(const char *const path);


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
