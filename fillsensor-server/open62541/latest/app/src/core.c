#include <argp.h>
#include <signal.h>
#include <stdlib.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include "tank.h"
#include "utils.h"


/*
 * Signal handling
 */
static volatile UA_Boolean running = true;

static void stopHandler(int signum)
{
    running = false;
}

/*
 * Argparser
 */
const char* argp_program_version = "fillsensor-server 0.1";
const char* argp_program_bug_address = "las3@oth-regensburg.de";
static char doc[] = "OPC UA server -- simulates a sensor for water level measurements";
static char args_doc[] = "";
static struct argp_option options[] = {
    {0},
};

struct arguments
{
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    switch(key)
    {
         default: {
            return ARGP_ERR_UNKNOWN;
        }
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


int main(int argc, char **argv)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /*
     * Default arguments
     */
    struct arguments arguments = {
    };
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    UA_StatusCode retval = 0;

    /*
     * Create and setup server
     */
    UA_Server *server = UA_Server_new();
    if(!server)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create sensor server");
        retval = UA_STATUSCODE_BAD;
        goto cleanup;
    }

    /*
     * Prepare the tank instance on the server with initial values
     */
    UA_QualifiedName qn;
    UA_NodeId tank1Ident;
    retval = defineWaterTankObjectType(server);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to define water tank object type");
        goto cleanup_server;
    }

    retval = addWaterTankObjectInstance(server, "tank1", &tank1Ident);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add water tank instance to server");
        goto cleanup_server;
    }

    qn = UA_QUALIFIEDNAME(1, "DeviceID");
    UA_NodeId deviceIdNode;
    retval = findAttributeNodeId(server, &tank1Ident, &qn, &deviceIdNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'DeviceID");
        goto cleanup_server;
    }
    UA_String deviceId = UA_STRING("T1");
    UA_Variant deviceIdValue;
    UA_Variant_setScalar(&deviceIdValue, &deviceId, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, deviceIdNode, deviceIdValue);

    qn = UA_QUALIFIEDNAME(1, "Location");
    UA_NodeId locationNode;
    retval = findAttributeNodeId(server, &tank1Ident, &qn, &locationNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'Location'");
        goto cleanup_server;
    }
    UA_String location = UA_STRING("P01B02R12"); // Plant 01 - Building 02 - Room 12
    UA_Variant locationValue;
    UA_Variant_setScalar(&locationValue, &location, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, locationNode, locationValue);

    qn = UA_QUALIFIEDNAME(1, "Capacity");
    UA_NodeId capacityNode;
    retval = findAttributeNodeId(server, &tank1Ident, &qn, &capacityNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'Capacity'");
        goto cleanup_server;
    }
    UA_Double capacity = 1000.;
    UA_Variant capacityValue;
    UA_Variant_setScalar(&capacityValue, &capacity, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, capacityNode, capacityValue);

    qn = UA_QUALIFIEDNAME(1, "FillPercentage");
    UA_NodeId fillPercentageNode;
    retval = findAttributeNodeId(server, &tank1Ident, &qn, &fillPercentageNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'FillPercentage'");
        goto cleanup_server;
    }
    UA_Double fillPercentage = 0.;
    UA_Variant fillPercentageValue;
    UA_Variant_setScalar(&fillPercentageValue, &fillPercentage, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, fillPercentageNode, fillPercentageValue);

    /*
     * Start event loop unless Ctrl-C has already been received
     */
    if(!running) goto cleanup_server;
    retval = UA_Server_run(server, &running);

cleanup_server:
    UA_Server_delete(server);

cleanup:
    return retval = UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
