import * as opcua from 'node-opcua';
import { program } from 'commander';
import bcrypt from 'bcryptjs';
import sqlite3 from 'sqlite3';
import { open, type Database } from 'sqlite';

type ExtendedUAObject = opcua.UAObject & {
  valvePosition: opcua.UAVariable
  fillPercentage: opcua.UAVariable
  threshold: opcua.UAVariable
};

async function getLastRowValue(
  db: Database<sqlite3.Database, sqlite3.Statement>,
  tableName: "waterlevel" | "valveposition" | "triggerthreshold",
  columnName: "level" | "position" | "threshold"
): Promise<null | number | boolean> {
  const row = await db.get<null | { [key: string]: (number | boolean) } | undefined>(`SELECT ${columnName} FROM ${tableName} ORDER BY id DESC LIMIT 1`);

  if (row === undefined || row === null) {
    console.error(`Error reading from ${tableName}`);
    return null;
  }

  console.log(`Read from ${tableName}:`, row);

  return row[columnName];
}

const salt = bcrypt.genSaltSync(10);
const users = [
  {
    username: "user1",
    password: bcrypt.hashSync("password1", salt),
    role: opcua.makeRoles([opcua.WellKnownRoles.AuthenticatedUser, opcua.WellKnownRoles.ConfigureAdmin]),
  },
  {
    username: "user2",
    password: bcrypt.hashSync("password2", salt),
    role: opcua.makeRoles([opcua.WellKnownRoles.AuthenticatedUser, opcua.WellKnownRoles.Operator]),
  },
];

const userManager = {
  isValidUser(username: string, password: string) {
    const user = users.find(u => u.username === username);
    return user && bcrypt.compareSync(password, user.password);
  },
  getUserRoles(username: string) {
    const user = users.find(u => u.username === username);
    return user ? user.role : [];
  },
} as opcua.UserManagerOptions

type MethodProps = opcua.AddMethodOptions & {
  namespace: opcua.Namespace;
  parent: opcua.UAObject | opcua.UAObjectType;
  method?: opcua.MethodFunctor;
};

function addMethod(methodProps: MethodProps) {
  return methodProps.namespace.addMethod(methodProps.parent, {
    browseName: methodProps.browseName,
    inputArguments: methodProps.inputArguments ?? [],
    outputArguments: methodProps.outputArguments ?? [],
    modellingRule: "Mandatory",
    ...methodProps.method
  });
}

type VariableProps = {
  namespace: opcua.Namespace;
  tankSystemType: opcua.UAObjectType;
  name: string;
  dataType: keyof typeof opcua.DataType;
  initialValue: boolean | number;
};

function addVariable(variableProps: VariableProps) {
  return variableProps.namespace.addVariable({
    componentOf: variableProps.tankSystemType,
    browseName: variableProps.name,
    dataType: variableProps.dataType,
    modellingRule: "Mandatory",
    value: {
      get: () => new opcua.Variant({ dataType: opcua.DataType[variableProps.dataType], value: variableProps.initialValue }),
    },
    minimumSamplingInterval: 1000
  });
}

type ServerProps = {
  uri: string;
  database: string;
};

async function server(serverProps: ServerProps) {

  console.table({
    certificateFile: process.env.CERT_FILE || "/pki/cert.pem",
    privateKeyFile: process.env.PRIVATE_KEY_FILE || "/pki/key.pem",
    ndoeEnv: process.env.NODE_ENV || "development",
  });

  const server = new opcua.OPCUAServer({
    host: serverProps.uri.split(":")[1].replace(/\/\//, ""),
    port: parseInt(serverProps.uri.split(":")[2]),
    buildInfo: {
      productName: "OpcUaServer",
      buildNumber: "69",
      buildDate: new Date()
    },
    serverInfo: {
      applicationUri: "urn:opcua-server.local",
      applicationName: { text: "OPC UA Server", locale: "en" },
    },
    securityPolicies: [
      // opcua.SecurityPolicy.None
      opcua.SecurityPolicy.Basic256Sha256,
    ],
    certificateFile: process.env.CERT_FILE || "/pki/cert.pem",
    privateKeyFile: process.env.PRIVATE_KEY_FILE || "/pki/key.pem",
    securityModes: [
      // opcua.MessageSecurityMode.None
      opcua.MessageSecurityMode.SignAndEncrypt
    ],
    userManager: userManager,
    // allowAnonymous: true,
  });

  const db = await open({
    filename: serverProps.database,
    driver: sqlite3.Database,
  });

  if (process.env.NODE_ENV === "development") {
    // Create tables if not exists
    await db.exec(`
    CREATE TABLE IF NOT EXISTS waterlevel (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        level REAL NOT NULL
    );
    CREATE TABLE IF NOT EXISTS valveposition (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        position INTEGER NOT NULL
    );
    CREATE TABLE IF NOT EXISTS triggerthreshold (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        threshold INTEGER NOT NULL
    );
  `);
  }

  await server.initialize();

  const addressSpace = server.engine.addressSpace;
  if (!addressSpace) {
    throw new Error("addressSpace not found");
  }

  const namespace = addressSpace.getOwnNamespace();

  const tankSystemType = namespace.addObjectType({
    browseName: "tankSystemType"
  });

  const fillPercentage = "FillPercentage";
  const valvePosition = "ValvePosition";
  const threshold = "Threshold";

  addVariable({
    namespace,
    tankSystemType,
    name: fillPercentage,
    dataType: "Double",
    initialValue: 0,
  });

  addVariable({
    namespace,
    tankSystemType,
    name: valvePosition,
    dataType: "Boolean",
    initialValue: false,
  });

  addVariable({
    namespace,
    tankSystemType,
    name: threshold,
    dataType: "Int32",
    initialValue: 0
  });

  const tankSystem1 = tankSystemType.instantiate({
    organizedBy: addressSpace.rootFolder.objects,
    browseName: "tankSystem1",
    namespace: namespace
  });

  const getTankSystemParams = addMethod({
    namespace,
    parent: tankSystem1,
    browseName: "getTankSystemParams",
    outputArguments: [
      {
        name: fillPercentage,
        dataType: opcua.DataType.Double
      },
      {
        name: valvePosition,
        dataType: opcua.DataType.Boolean
      },
      {
        name: threshold,
        dataType: opcua.DataType.Int32
      }
    ],
  });

  getTankSystemParams.bindMethod(async (_, context, callback) => {
    // Write to the server values 
    const fillPercentage = { dataType: opcua.DataType.Double, value: NaN } as opcua.Variant;
    const valvePosition = { dataType: opcua.DataType.Boolean, value: undefined } as opcua.Variant;
    const threshold = { dataType: opcua.DataType.Int32, value: NaN } as opcua.Variant;

    // get from sqlite db
    const fillPercentageValue = await getLastRowValue(db, "waterlevel", "level");
    if (fillPercentageValue === null) {
      console.error(`No data found or failed to step fill percentage`);
      callback(null, {
        statusCode: opcua.StatusCodes.BadOutOfRange
      });
      return;
    }
    fillPercentage.value = fillPercentageValue
    const valvePositionValue = await getLastRowValue(db, "valveposition", "position");
    if (valvePositionValue === null) {
      console.error(`No data found or failed to step valve position`);
      callback(null, {
        statusCode: opcua.StatusCodes.BadOutOfRange
      });
      return;
    }
    valvePosition.value = Boolean(valvePositionValue);
    const thresholdValue = await getLastRowValue(db, "triggerthreshold", "threshold");
    if (thresholdValue === null) {
      console.error(`No data found or failed to step threshold`);
      callback(null, {
        statusCode: opcua.StatusCodes.BadOutOfRange
      });
      return;
    }
    threshold.value = thresholdValue;

    const uaObject = context.object as ExtendedUAObject;

    uaObject.fillPercentage.setValueFromSource(fillPercentage);
    uaObject.threshold.setValueFromSource(threshold);
    uaObject.valvePosition.setValueFromSource(valvePosition);

    callback(null, {
      statusCode: opcua.StatusCodes.Good,
      outputArguments: [
        valvePosition,
        fillPercentage,
        threshold,
      ]
    });
  });

  const setThreshold = addMethod({
    namespace,
    parent: tankSystem1,
    browseName: "setThreshold",
    inputArguments: [
      {
        name: threshold,
        dataType: opcua.DataType.Int32
      }
    ],
  });

  setThreshold.bindMethod(async (inputArguments, context, callback) => {
    const threshold = inputArguments[0].value as number;
    if (typeof threshold !== "number" || inputArguments.length !== 1) {
      console.error("Received data with wrong datatype or dimension");
      callback(null, {
        statusCode: opcua.StatusCodes.BadInvalidArgument,
      });
      return;
    }

    const uaObject = context.object as ExtendedUAObject;

    // write to sqlite db
    const result = await db.run("INSERT INTO triggerthreshold (threshold) VALUES (?) ", [threshold]);
    if (result.changes !== 1) {
      console.error("Failed to write threshold to database");
      callback(null, {
        statusCode: opcua.StatusCodes.BadInternalError,
      });
    }

    uaObject.threshold.setValueFromSource({ dataType: opcua.DataType.Int32, value: threshold });
    callback(null, {
      statusCode: opcua.StatusCodes.Good
    });
  });

  server.start(function() {
    console.log("Server is now listening ... (press CTRL+C to stop)");
    console.log("port ", server.endpoints[0].port);
  });
}

(() => {
  program
    .version("1.0.0 - Waifu Edition")
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
