#include <argp.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
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
const char* argp_program_version = "plc-logic-client 0.1";
const char* argp_program_bug_address = "las3@oth-regensburg.de";
static char doc[] = "PLC Logic Module -- Simulates process control unit";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"sensor-uri",   's', "URL",  0, "Sensor URI <opc.tcp://hostname:port>" },
    {"actuator-uri", 'a', "URL",  0, "Acutator URI <opc.tcp://hostname:port>" },
    {"database",     'd', "PATH", 0, "Path to the SQLite database" },
    {0},
};

struct arguments
{
    char *suri;
    char *auri;
    char *dbname;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    switch(key)
    {
        case 's': {
            arguments->suri = arg;
            break;
        }
        case 'a': {
            arguments->auri = arg;
            break;
        }
        case 'd': {
            arguments->dbname = arg;
            break;
        }
        default: {
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
    UA_Client *aclient;
    UA_NodeId openNodeId;
} CallbackContext;

/*
 * Callback when receiving a value change from the sensor
 */
static void valueChangedCallback(UA_Client *client, UA_UInt32 subId, void *subContext,
                                UA_UInt32 monId, void *monContext, UA_DataValue *value)
{
    CallbackContext *context = (CallbackContext *)monContext;
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE]))
    {
        /*
         * Write the new fill percentage value to database
         */
        UA_Double fillPercentage = *(UA_Double *)value->value.data;
        const char *sql = "INSERT INTO waterlevel (level) VALUES (?)";
        sqlite3_stmt *stmt;
        if(sqlite3_prepare_v2(context->db, sql, -1, &stmt, NULL) == SQLITE_OK)
        {
            sqlite3_bind_double(stmt, 1, fillPercentage);
            if(sqlite3_step(stmt) != SQLITE_DONE)
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Could not write waterlevel to database with error: %s",
                               sqlite3_errmsg(context->db));
            }
            sqlite3_finalize(stmt);
        }
        else
        {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Failed to prepare SQL statement with error: %s",
                           sqlite3_errmsg(context->db));
        }

        /*
         * Logic for setting valve open/closed
         */
        double upperThreshold = 75.; // percent
        double lowerThreshold = 25.; // percent
        static UA_Boolean valveOpen = false;

        if((!valveOpen) && (fillPercentage > upperThreshold))
        {
            valveOpen = true;
            UA_Variant *valveOpenValue = UA_Variant_new();
            if(UA_Variant_setScalarCopy(valveOpenValue, &valveOpen, &UA_TYPES[UA_TYPES_BOOLEAN]) != UA_STATUSCODE_GOOD)
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Unable to set valveOpen to %d",
                               valveOpen);
                return;
            }
            if(UA_Client_writeValueAttribute(context->aclient, context->openNodeId, valveOpenValue) != UA_STATUSCODE_GOOD)
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Unable to write valveOpen to server");
                return;
            }
            UA_Variant_delete(valveOpenValue);

            const char* sql = "INSERT INTO valveposition (position) VALUES (?)";
            if(sqlite3_prepare_v2(context->db, sql, -1, &stmt, NULL) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, (int)valveOpen);
                if(sqlite3_step(stmt) != SQLITE_DONE)
                {
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                   "Could not write valveposition to database with error: %s",
                                   sqlite3_errmsg(context->db));
                }
                sqlite3_finalize(stmt);
            }
            else
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Failed to prepare SQL statement with error: %s",
                               sqlite3_errmsg(context->db));
            }
        }
        else if(valveOpen && (fillPercentage < lowerThreshold))
        {
            valveOpen = false;
            UA_Variant *valveOpenValue = UA_Variant_new();
            if(UA_Variant_setScalarCopy(valveOpenValue, &valveOpen, &UA_TYPES[UA_TYPES_BOOLEAN]) != UA_STATUSCODE_GOOD)
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Unable to set valveOpen to %d",
                               valveOpen);
                return;
            }
            UA_Client_writeValueAttribute(context->aclient, context->openNodeId, valveOpenValue);
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Unable to write valveOpen to server");
                return;
            }
            UA_Variant_delete(valveOpenValue);

            const char* sql = "INSERT INTO valveposition (position) VALUES (?)";
            if(sqlite3_prepare_v2(context->db, sql, -1, &stmt, NULL) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, (int)valveOpen);
                if(sqlite3_step(stmt) != SQLITE_DONE)
                {
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                   "Could not write valveposition to database with error: %s",
                                   sqlite3_errmsg(context->db));
                }
                sqlite3_finalize(stmt);
            }
            else
            {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Failed to prepare SQL statement with error: %s",
                               sqlite3_errmsg(context->db));
            }
        }
    }
    else
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Received data with wrong datatype");
    }
}


int main(int argc, char **argv)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /*
     * Default arguments
     */
    struct arguments arguments = {
        .suri = "opc.tcp://127.0.0.1:4840",
        .auri = "opc.tcp://127.0.0.1:4840",
        .dbname = "/db.sqlite3",
    };
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    UA_StatusCode retval = 0;

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
     * Create and setup clients
     */
    UA_Client *sclient = UA_Client_new();
    if(!sclient)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create sensor client");
        retval = UA_STATUSCODE_BAD;
        goto cleanup_db;
    }

    UA_Client *aclient = UA_Client_new();
    if(!aclient)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create actuator client");
        retval = UA_STATUSCODE_BAD;
        goto cleanup_sclient;
    }

    UA_ClientConfig *scfg = UA_Client_getConfig(sclient);
    retval = UA_ClientConfig_setDefault(scfg);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create sensor config");
        goto cleanup_aclient;
    }
    scfg->securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_ByteString_clear(&scfg->securityPolicyUri);
    scfg->securityPolicyUri = UA_String_fromChars(
        "http://opcfoundation.org/UA/SecurityPolicy#None");

    UA_ClientConfig *acfg = UA_Client_getConfig(aclient);
    retval = UA_ClientConfig_setDefault(acfg);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create actuator config");
        goto cleanup_aclient;
    }
    acfg->securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_ByteString_clear(&acfg->securityPolicyUri);
    acfg->securityPolicyUri = UA_String_fromChars(
        "http://opcfoundation.org/UA/SecurityPolicy#None");

    /*
     * Sensor client connect
     */
    retval = UA_Client_connect(sclient, arguments.suri);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to connect to sensor");
        goto cleanup_aclient;
    }

    /*
     * Actuator client connect
     */
    retval = UA_Client_connect(aclient, arguments.auri);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to connect to actuator");
        goto cleanup_sclient_disconnect;
    }
    /*
     * Request the node ID of the open attribute from the valve
     */
    UA_NodeId openNodeId;
    char *a_path[] = {"valve1", "Open"};
    UA_UInt32 ids[] = {UA_NS0ID_ORGANIZES, UA_NS0ID_HASCOMPONENT};
    retval = translateBrowsePathToNodeIdRequest(aclient, &openNodeId, a_path, ids, 2);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to retrieve node ID");
        goto cleanup_aclient_disconnect;
    }
    /*
     * Request the node ID of the fillPercentage attribute from sensor
     */
    UA_NodeId fillPctNodeId;
    char *s_path[] = {"tank1", "FillPercentage"};
    retval = translateBrowsePathToNodeIdRequest(sclient, &fillPctNodeId, s_path, ids, 2);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to retrieve node ID");
        goto cleanup_aclient_disconnect;
    }

    /*
     * Set up the subscription on the sensor server
     */
    CallbackContext context = {
        .db = db,
        .aclient = aclient,
        .openNodeId = openNodeId,
    };

    UA_CreateSubscriptionRequest subRequest = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResponse =
        UA_Client_Subscriptions_create(sclient, subRequest, NULL, NULL, NULL);
    if(subResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable to create subscription");
        goto cleanup_aclient_disconnect;
    }

    /*
     * Set up the monitored item on the sensor server
     */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(fillPctNodeId);
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(
            sclient,
            subResponse.subscriptionId,
            UA_TIMESTAMPSTORETURN_BOTH,
            monRequest,
            &context,
            valueChangedCallback,
            NULL);
    if(monResponse.statusCode != UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Unable add monitored item to subscription");
        goto cleanup_aclient_disconnect;
    }

    /*
     * Run the eventloop unless Ctrl-C has already been received
     */
    if(!running) goto cleanup_aclient_disconnect;
    while(running && UA_Client_run_iterate(sclient, 1000) == UA_STATUSCODE_GOOD);


cleanup_aclient_disconnect:
    UA_Client_disconnect(aclient);

cleanup_sclient_disconnect:
    UA_Client_disconnect(sclient);

cleanup_aclient:
    UA_Client_delete(aclient);

cleanup_sclient:
    UA_Client_delete(sclient);

cleanup_db:
    sqlite3_close(db);

cleanup:
    return retval = UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
