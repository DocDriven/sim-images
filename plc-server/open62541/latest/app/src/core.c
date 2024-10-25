#include <argp.h>
#include <open62541/common.h>
#include <open62541/plugin/log.h>
#include <open62541/types.h>
#include <open62541/util.h>
#include <sqlite3.h>
#include <signal.h>
#include <stdlib.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "tank_system.h"
#include "utils.h"


#define MAX_SIZE_TRUSTLIST 10
#define MAX_SIZE_ISSUERLIST 10

static size_t trustListSize = 0;
static size_t issuerListSize = 0;

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
const char* argp_program_version = "plc-server 0.1";
const char* argp_program_bug_address = "las3@oth-regensburg.de";
static char doc[] = "OPC UA Server -- providing encrypted access to database contents";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"encrypt",     'e', 0,      0, "Enable encryption" },
    {"certificate", 'c', "FILE", 0, "Server certificate" },
    {"key",         'k', "FILE", 0, "Private key" },
    {"trustlist",   't', "FILE", 0, "Trust list" },
    {"issuerlist",  'i', "FILE", 0, "Issuer list" },
    {"database",    'd', "PATH", 0, "Path to the SQLite database" },
    { 0 }
};

struct arguments
{
    char *dbname;
    char *cert;
    char *private;
    char *trustlist[MAX_SIZE_TRUSTLIST];
    char *issuerlist[MAX_SIZE_ISSUERLIST];
    int encrypt;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch(key)
    {
        case 'd':
        {
            arguments->dbname = arg;
            break;
        }
        case 'c':
        {
            arguments->cert = arg;
            break;
        }
        case 'k':
        {
            arguments->private = arg;
            break;
        }
        case 'e':
        {
            arguments->encrypt = 1;
            break;
        }
        case 't':
        {
            if( trustListSize < MAX_SIZE_TRUSTLIST )
            {
                arguments->trustlist[trustListSize++] = arg;
            }
            else
            {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Max number of trustlist entries reached. Ignoring %s",
                            arg);
            }
            break;
        }
        case 'i':
        {
            if( issuerListSize < MAX_SIZE_ISSUERLIST )
            {
                arguments->issuerlist[issuerListSize++] = arg;
            }
            else
            {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Max number of issuerlist entries reached. Ignoring %s",
                            arg);
            }
            break;
        }
        default:
        {
            return ARGP_ERR_UNKNOWN;
        }
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

/*
 * Structure needed to pass objects to callbacks
 */
typedef struct {
    sqlite3 *db;
    UA_NodeId fillPctNodeIdent;
    UA_NodeId valvePosNodeIdent;
    UA_NodeId thresholdNodeIdent;
} CallbackContext;


/*
 * Callback when getTankSystemParams method is invoked
 */
static UA_StatusCode getTankSystemParamsCallback(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *methodId, void *methodContext,
    const UA_NodeId *objectId, void *objectContext,
    size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output)
{
    CallbackContext *context = (CallbackContext*)methodContext;

    /*
     * Initialize the output array
     */
    UA_Variant *nullArray = (UA_Variant*)UA_Array_new(3, &UA_TYPES[UA_TYPES_VARIANT]);
    for(size_t iter = 0; iter < 3; iter++) UA_Variant_init(&nullArray[iter]);
    UA_Variant_setArrayCopy(&output[0], nullArray, 3, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_Array_delete(nullArray, 3, &UA_TYPES[UA_TYPES_VARIANT]);

    if(inputSize != 0)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Received data with wrong datatype or dimension");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /*
     * Prepare statements and execute for FillPercentage
     */
    const char *sqlFillPct = "SELECT level FROM waterlevel ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmtFillPct;
    if(sqlite3_prepare_v2(context->db, sqlFillPct, -1, &stmtFillPct, NULL) != SQLITE_OK)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Failed to prepare SQL statement for fill percentage with error: %s",
                    sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Double fillPct = 0.;
    if(sqlite3_step(stmtFillPct) == SQLITE_ROW)
    {
        fillPct = sqlite3_column_double(stmtFillPct, 0);
    }
    else
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "No data found or failed to step fill percentage: %s",
                     sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADOUTOFRANGE;
    }
    sqlite3_finalize(stmtFillPct);

    /*
     * Prepare statements and execute for ValvePosition
     */
    const char *sqlValvePos = "SELECT position FROM valveposition ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmtValvePos;
    if(sqlite3_prepare_v2(context->db, sqlValvePos, -1, &stmtValvePos, NULL) != SQLITE_OK)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Failed to prepare SQL statement for valve position with error: %s",
                    sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Boolean valvePos = UA_FALSE;
    if(sqlite3_step(stmtValvePos) == SQLITE_ROW)
    {
        valvePos = (sqlite3_column_int(stmtValvePos, 0) != 0) ? UA_TRUE : UA_FALSE;
    }
    else
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "No data found or failed to step valve position: %s",
                     sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADOUTOFRANGE;
    }
    sqlite3_finalize(stmtValvePos);

    /*
     * Prepare statements and execute for Threshold
     */
    const char *sqlThreshold = "SELECT threshold FROM triggerthreshold ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmtThreshold;
    if(sqlite3_prepare_v2(context->db, sqlThreshold, -1, &stmtThreshold, NULL) != SQLITE_OK)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to prepare SQL statement for threshold with error: %s",
                     sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Int32 threshold = 0;
    if(sqlite3_step(stmtThreshold) == SQLITE_ROW)
    {
        threshold = sqlite3_column_int(stmtThreshold, 0);
    }
    else
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "No data found or failed to step threshold: %s",
                     sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADOUTOFRANGE;
    }
    sqlite3_finalize(stmtThreshold);

    /*
     * Write the retrieved values to the server
     */
    UA_Variant fillPercentageValue;
    UA_Variant_setScalar(&fillPercentageValue, &fillPct, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, context->fillPctNodeIdent, fillPercentageValue);

    UA_Variant valvePositionValue;
    UA_Variant_setScalar(&valvePositionValue, &valvePos, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, context->valvePosNodeIdent, fillPercentageValue);

    UA_Variant thresholdValue;
    UA_Variant_setScalar(&thresholdValue, &threshold, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, context->thresholdNodeIdent, thresholdValue);

    /*
     * Return the retrieved values to client
     */
    UA_Variant_setScalarCopy(&output[0], &fillPct, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Variant_setScalarCopy(&output[1], &valvePos, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalarCopy(&output[2], &threshold, &UA_TYPES[UA_TYPES_INT32]);

    return UA_STATUSCODE_GOOD;
}


/*
 * Callback when setThreshold method is invoked
 */
static UA_StatusCode setThresholdCallback(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *methodId, void *methodContext,
    const UA_NodeId *objectId, void *objectContext,
    size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output)
{
    /*
     * Input validation
     */
    CallbackContext *context = (CallbackContext*)methodContext;
    if(inputSize != 1 || !UA_Variant_hasScalarType(input, &UA_TYPES[UA_TYPES_INT32]))
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Received data with wrong datatype or dimension");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_Int32 newThreshold = *(UA_Int32*)input->data;

    /*
     * Prepare statements and execute for Threshold
     */
    const char *sql = "INSERT INTO triggerthreshold (threshold) VALUES (?)";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(context->db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Failed to prepare SQL statement with error: %s",
                    sqlite3_errmsg(context->db));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(sqlite3_bind_int(stmt, 1, newThreshold) != SQLITE_OK)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to bind value with error: %s",
                     sqlite3_errmsg(context->db));
        sqlite3_finalize(stmt);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not write threshold to database with error: %s",
                     sqlite3_errmsg(context->db));
        sqlite3_finalize(stmt);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    sqlite3_finalize(stmt);

    /*
     * Write the new value to the server
     */
    UA_Variant newThresholdValue;
    UA_Variant_setScalar(&newThresholdValue, &newThreshold, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, context->thresholdNodeIdent, newThresholdValue);

    return UA_STATUSCODE_GOOD;
}


int main(int argc, char *argv[])
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /*
     * Default arguments
     */
    struct arguments arguments = {
        .dbname = "/db.sqlite3",
        .cert = "",
        .private = "",
        .trustlist = {""},
        .issuerlist = {""},
        .encrypt = false,
    };
    argp_parse(&argp, argc, argv, 0, 0, &arguments);


    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString cert = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;

    /*
     * Open the database
     */
    sqlite3 *db;
    if(sqlite3_open(arguments.dbname, &db))
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to open database");
        retval = UA_STATUSCODE_BAD;
        goto cleanup;
    }
    /*
     * Create and setup server
     */
    UA_Server *server = UA_Server_new();
    if(!server)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create plc server");
        retval = UA_STATUSCODE_BAD;
        goto cleanup_db;
    }

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    retval = UA_ServerConfig_setDefault(cfg);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create server config");
        goto cleanup_server;
    }

    /*
     * Set up config for encryption
     */
    if(arguments.encrypt)
    {
        if( (strlen(arguments.cert) == 0) || (strlen(arguments.private) == 0) )
        {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Certificate and/or key missing");
            goto cleanup_server;
        }

        cert = loadFile(arguments.cert);
        if(!cert.length)
        {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Certificate is empty");
            goto cleanup_server;
        }

        privateKey = loadFile(arguments.private);
        if(!privateKey.length)
        {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Private key is empty");
            goto cleanup_server;
        }

        /*
         * Omitting checks for validity of these list entries for now
         * due to them not being relevant right now
         */
        UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
        for(size_t idx = 0; idx < trustListSize; idx++)
        {
            trustList[idx] = loadFile(arguments.trustlist[idx]);
        }

        UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
        for(size_t idx = 0; idx < issuerListSize; idx++)
        {
            issuerList[idx] = loadFile(arguments.issuerlist[idx]);
        }

        /*
         * Check for success happens later after deleting the other variables
         */
        retval = UA_ServerConfig_setDefaultWithSecurityPolicies(
            cfg, 4840, &cert, &privateKey,
            trustList, trustListSize,
            issuerList, issuerListSize,
            NULL, 0);

        UA_ByteString_clear(&cert);
        UA_ByteString_clear(&privateKey);

        for(size_t idx = 0; idx < trustListSize; idx++)
        {
            UA_ByteString_clear(&trustList[idx]);
        }

        for(size_t idx = 0; idx < issuerListSize; idx++)
        {
            UA_ByteString_clear(&issuerList[idx]);
        }

        /*
         * Check for success now
         */
        if(retval != UA_STATUSCODE_GOOD)
        {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Setting encryption configuration failed: %s",
                        UA_StatusCode_name(retval));
            goto cleanup_server;
        }
    }

    /*
     * Prepare the system instance on the server with initial values
     */
    UA_QualifiedName qn;
    UA_NodeId tankSystem1Ident;
    retval = defineTankSystemObjectType(server);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to define tank system object type");
        goto cleanup_server;
    }

    retval = addTankSystemObjectInstance(server, "tankSystem1", &tankSystem1Ident);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add tank system instance to server");
        goto cleanup_server;
    }

    qn = UA_QUALIFIEDNAME(1, "FillPercentage");
    UA_NodeId fillPercentageNode;
    retval = findAttributeNodeId(server, &tankSystem1Ident, &qn, &fillPercentageNode);
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

    qn = UA_QUALIFIEDNAME(1, "ValvePosition");
    UA_NodeId valvePositionNode;
    retval = findAttributeNodeId(server, &tankSystem1Ident, &qn, &valvePositionNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'ValvePosition'");
        goto cleanup_server;
    }
    UA_Boolean valvePosition = false;
    UA_Variant valvePositionValue;
    UA_Variant_setScalar(&valvePositionValue, &valvePosition, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, valvePositionNode, valvePositionValue);

    qn = UA_QUALIFIEDNAME(1, "Threshold");
    UA_NodeId thresholdNode;
    retval = findAttributeNodeId(server, &tankSystem1Ident, &qn, &thresholdNode);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to find attribute 'Threshold'");
        goto cleanup_server;
    }
    UA_Int32 threshold = 0;
    UA_Variant thresholdValue;
    UA_Variant_setScalar(&thresholdValue, &threshold, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, thresholdNode, thresholdValue);

    /*
     * Create methods
     */
    CallbackContext context = {
        .db = db,
        .fillPctNodeIdent = fillPercentageNode,
        .valvePosNodeIdent = valvePositionNode,
        .thresholdNodeIdent = thresholdNode,
    };

    // getTankSystemParams method
    UA_Argument outputArgument[3];

    UA_Argument_init(&outputArgument[0]);
    outputArgument[0].description = UA_LOCALIZEDTEXT("en-US", "Fill percentage of the water tank");
    outputArgument[0].name = UA_STRING("FillPercentage");
    outputArgument[0].dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    outputArgument[0].valueRank = UA_VALUERANK_SCALAR;

    UA_Argument_init(&outputArgument[1]);
    outputArgument[1].description = UA_LOCALIZEDTEXT("en-US", "Chemical valve position");
    outputArgument[1].name = UA_STRING("ValvePosition");
    outputArgument[1].dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    outputArgument[1].valueRank = UA_VALUERANK_SCALAR;

    UA_Argument_init(&outputArgument[2]);
    outputArgument[2].description = UA_LOCALIZEDTEXT("en-US", "Threshold value for logic");
    outputArgument[2].name = UA_STRING("Threshold");
    outputArgument[2].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArgument[2].valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes mAttrGet = UA_MethodAttributes_default;
    mAttrGet.description = UA_LOCALIZEDTEXT("en-US", "Get all relevant tank system parameters");
    mAttrGet.displayName = UA_LOCALIZEDTEXT("en-US", "getTankSystemParams");
    mAttrGet.executable = true;
    mAttrGet.userExecutable = true;


    retval = UA_Server_addMethodNode(
        server,
        UA_NODEID_NULL,
        tankSystem1Ident,
        UA_NS0ID(HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "getTankSystemParams"),
        mAttrGet,
        &getTankSystemParamsCallback,
        0, NULL,
        3, outputArgument,
        &context, NULL);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add method 'getTankSystemParams'");
        goto cleanup_server;
    }

    // setThreshold method
    UA_Argument inputArgument[1];

    UA_Argument_init(&inputArgument[0]);
    inputArgument[0].description = UA_LOCALIZEDTEXT("en-US", "Threshold value for PLC logic");
    inputArgument[0].name = UA_STRING("Threshold");
    inputArgument[0].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArgument[0].valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes mAttrSet = UA_MethodAttributes_default;
    mAttrSet.description = UA_LOCALIZEDTEXT("en-US", "Set threshold value for PLC logic");
    mAttrSet.displayName = UA_LOCALIZEDTEXT("en-US", "setThreshold");
    mAttrSet.executable = true;
    mAttrSet.userExecutable = true;

    retval = UA_Server_addMethodNode(
        server,
        UA_NODEID_NULL,
        tankSystem1Ident,
        UA_NS0ID(HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "setThreshold"),
        mAttrSet,
        &setThresholdCallback,
        1, inputArgument,
        0, NULL,
        &context, NULL);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to add method 'setThreshold'");
        goto cleanup_server;
    }

    /*
     * Start event loop unless Ctrl-C has already been received
     */
    if(!running) goto cleanup_server;
    retval = UA_Server_run(server, &running);

cleanup_server:
    UA_Server_delete(server);

cleanup_db:
    sqlite3_close(db);

cleanup:
    return retval = UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
