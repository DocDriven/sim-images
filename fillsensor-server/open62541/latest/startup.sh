#!/bin/sh

# treat undefined variables as an error
set -u

# if no ENV is set, the binary is started with defaults
/usr/local/bin/fillsensor-server
