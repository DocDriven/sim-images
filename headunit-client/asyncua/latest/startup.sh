#!/bin/sh

# Treat unset variables as an error when substituting.
set -u

# if OPCUA_SERVER_ADDRESS is set, then use it instead
server_uri_opt=""
if [ ! -z $SERVER_URI ]; then
    server_uri_opt="--url "
fi

# if INTERVAL is set, then use it instead
interval_opt=""
if [ ! -z $INTERVAL ]; then
    interval_opt="--interval "
fi

# Create the database if it does not exist yet
DBNAME="/usr/src/app/data.sqlite3"

# Define the SQL command for creating the table
CREATE_DATA_TABLE="
CREATE TABLE IF NOT EXISTS tanksystemdata (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    level REAL NOT NULL,
    position INTEGER NOT NULL,
    threshold INTEGER NOT NULL
);
"

sqlite3 "$DBNAME" <<EOF
$CREATE_DATA_TABLE
EOF


/pki/gen_kc_pair.sh

python3.11 /usr/src/app/vis.py &

python3.11 /usr/src/app/client.py \
    ${server_uri_opt}${SERVER_URI} \
    ${interval_opt}${INTERVAL} \
    --database ${DB_NAME}
