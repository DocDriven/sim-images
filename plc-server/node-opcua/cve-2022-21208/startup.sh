#!/bin/sh

# treat undefined variables as an error
set -u

# wait until another process has setup the database
# including creating the tables
DB_NAME="${DB_NAME:-/database/process_database.sqlite3}"
LOCKFILE="${DB_NAME:-/database/process_database.sqlite3}.lock"
while [ ! -f "$LOCKFILE" ]; do
  sleep 1
done

# generate the keys for encryption
/pki/gen_kc_pair.sh

# start the server
cd /app && npm start -- --database "$DB_NAME"
