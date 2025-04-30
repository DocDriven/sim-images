#!/bin/sh

# treat undefined variables as an error
set -u

# if SERVER_URI is set, then use it instead
server_uri_opt=""
if [ -n "$SERVER_URI" ]; then
  server_uri_opt="--server="
fi

# if no ENV is set, the binary is started with defaults
# the missing space for addresses is on purpose, as the
# prefix opc.mqtt:// is included in the option variable

# start the simulation
python3 /app/sim.py "${server_uri_opt}${SERVER_URI}"
