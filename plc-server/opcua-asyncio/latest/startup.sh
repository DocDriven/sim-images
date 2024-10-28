#!/bin/sh

# treat undefined variables as an error
set -u

# wait until another process has setup the database
# including creating the tables
LOCKFILE="/database/process_database_ready.lock"
while [ ! -f $LOCKFILE ]; do
  sleep 1
done

# generate the keys for encryption
cd /pki && /bin/bash gen_kc_pair.sh

# start the server
python3.11 /app/server.py --database /database/process_database.sqlite3
