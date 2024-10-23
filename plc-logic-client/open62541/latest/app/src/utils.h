#ifndef UTILS_H
#define UTILS_H

#include <open62541/client.h>

/*
 * Send request to translate path to node ID to server
 */
UA_StatusCode translateBrowsePathToNodeIdRequest(
    UA_Client *client,
    UA_NodeId *nodeId,
    char* path[],
    UA_UInt32 id[],
    int len);

#endif
