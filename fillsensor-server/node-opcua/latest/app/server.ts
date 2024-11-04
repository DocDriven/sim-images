import * as opcua from 'node-opcua';
import { program } from 'commander';

type ExtendedUAObject = opcua.UAObject & {
    valvePosition: opcua.UAVariable
    fillPercentage: opcua.UAVariable
    threshold: opcua.UAVariable
};

type ServerProps = {
    uri: string;
    database: string;
};

async function server(serverProps: ServerProps) {

    var opcua = require("node-opcua");

    console.table({
        certificateFile: process.env.CERT_FILE || "/pki/cert.pem",
        privateKeyFile: process.env.PRIVATE_KEY_FILE || "/pki/key.pem",
        ndoeEnv: process.env.NODE_ENV || "development",
    });

    var default_nodeset ="WS.NodeSet2.xml";
    const nodesets = require("node-opcua-nodesets");
    const server = new opcua.OPCUAServer({
        host: serverProps.uri.split(":")[1].replace(/\/\//, ""),
        port: parseInt(serverProps.uri.split(":")[2]),
        nodeset_filename: [
            nodesets.nodesets.standard,
            default_nodeset
          ],
        buildInfo: {
            productName: "OpcUaServer",
            buildNumber: "69",
            buildDate: new Date()
        },
        securityPolicies: [
            opcua.SecurityPolicy.None,
            opcua.SecurityPolicy.Basic256Sha256,
        ],
        // certificateFile: process.env.CERT_FILE || "/pki/cert.pem",
        // privateKeyFile: process.env.PRIVATE_KEY_FILE || "/pki/key.pem",
        securityModes: [
            opcua.MessageSecurityMode.None,
            opcua.MessageSecurityMode.SignAndEncrypt
        ]
    });

    await server.initialize();

    server.start(function () {
        console.log("Server is now listening ... (press CTRL+C to stop)");
        console.log("port ", server.endpoints[0].port);
    });
}

(() => {
    program
        .version("1.0.0 - Las3")
        .description("OPC UA Server")
        .option("-u, --uri <uri>", "Endpoint URI e.g. opc.tcp://<hostname>:<port>", "opc.tcp://0.0.0.0:4840")
        .option("-d, --database <database file>", "Database file", "./db.sqlite")
        .parse(process.argv);

    const { uri, database } = program.opts() as ServerProps;

    console.table({ uri, database });

    if (!uri || !database) {
        console.error("Missing required arguments");
        process.exit(1);
    }

    server({ uri, database });
})();