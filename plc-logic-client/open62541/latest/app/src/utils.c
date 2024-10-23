#include "utils.h"

UA_StatusCode translateBrowsePathToNodeIdRequest(
    UA_Client *client,
    UA_NodeId *nodeId,
    char* path[],
    UA_UInt32 id[],
    int len)
{
    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(len, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
    browsePath.relativePath.elementsSize = len;

    for(size_t i = 0; i < len; i++)
    {
        UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
        elem->referenceTypeId = UA_NODEID_NUMERIC(0, id[i]);
        elem->targetName = UA_QUALIFIEDNAME_ALLOC(1, path[i]);
    }

    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = &browsePath;
    request.browsePathsSize = 1;
    UA_TranslateBrowsePathsToNodeIdsResponse response =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, request);
    if(   response.responseHeader.serviceResult != UA_STATUSCODE_GOOD
       || response.resultsSize < 1
       || response.results[0].targetsSize < 1)
    {
        return response.responseHeader.serviceResult;
    }
    *nodeId = response.results[0].targets[0].targetId.nodeId;
    return UA_STATUSCODE_GOOD;
}
