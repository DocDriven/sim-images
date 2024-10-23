#include "utils.h"

UA_StatusCode findAttributeNodeId(
    UA_Server *server,
    const UA_NodeId *startNodeIdent,
    const UA_QualifiedName *qName,
    UA_NodeId *attributeNodeId)
{
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = *qName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *startNodeIdent;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1)
    {
        return bpr.statusCode;
    }
    *attributeNodeId = bpr.targets[0].targetId.nodeId;

    return UA_STATUSCODE_GOOD;
}

