#!/bin/sh

# treat undefined variables as an error
set -u

# if SENSOR_URI is set, then use it instead
sensor_uri_opt=""
if [ -n "$SENSOR_URI" ]; then
  sensor_uri_opt="--sensor-uri="
fi

# if ACTUATOR_URI is set, then use it instead
act_uri_opt=""
if [ -n "$ACTUATOR_URI" ]; then
  act_uri_opt="--actuator-uri="
fi

# create the database if it does not exist yet AND create
# a lock file to indicate to other containers that the
# database is ready to use
DB_NAME="${DB_NAME:-/database/process_database.sqlite3}"
LOCKFILE="${DB_NAME:-/database/process_database.sqlite3}.lock"
if [ ! -f "$LOCKFILE" ]; then

# Define the SQL commands for creating the tables
CREATE_WATERLEVEL_TABLE="
CREATE TABLE IF NOT EXISTS waterlevel (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    level REAL NOT NULL
);
"

CREATE_VALVEPOSITION_TABLE="
CREATE TABLE IF NOT EXISTS valveposition (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    position INTEGER NOT NULL
);
"

CREATE_TRIGGERTHRESHOLD_TABLE="
CREATE TABLE IF NOT EXISTS triggerthreshold (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    threshold INTEGER NOT NULL
);
"

sqlite3 "$DB_NAME" <<EOF
$CREATE_WATERLEVEL_TABLE
$CREATE_VALVEPOSITION_TABLE
$CREATE_TRIGGERTHRESHOLD_TABLE
EOF

# create the file to indicate to other containers that
# the database is ready to use
touch "$LOCKFILE"

fi

# if no ENV is set, the binary is started with defaults
# the missing space for addresses is on purpose, as the
# prefix opc.mqtt:// is included in the option variable
/usr/local/bin/plc-logic-client \
    "${sensor_uri_opt}${SENSOR_URI}" \
    "${act_uri_opt}${ACTUATOR_URI}" \
    --database="${DB_NAME}"
