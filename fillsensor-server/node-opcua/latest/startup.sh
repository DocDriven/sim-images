#!/bin/sh

# treat undefined variables as an error
set -u

# start the server
cd /app && npm start 