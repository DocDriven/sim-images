#include "utils.h"
#include <open62541/config.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include <stdio.h>


UA_ByteString loadFile(const char *const path)
{
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp)
    {
        /*
         * This returns an empty UA_ByteString to check for
         */
        return fileContents;
    }

    /*
     * Get the file length, allocate the data and read
     */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data)
    {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
        {
            UA_ByteString_clear(&fileContents);
        }
    }
    else
    {
        fileContents.length = 0;
    }
    fclose(fp);
    return fileContents;
}


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
