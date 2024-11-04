#!/bin/bash

# treat undefined variables as an error
set -u

# if no ENV is set, the binary is started with defaults
python3.11 app/server.py