#!/bin/sh

# treat undefined variables as an error
set -u

# start the server
/app/target/release/opcua-server-rust --config /app/server.conf