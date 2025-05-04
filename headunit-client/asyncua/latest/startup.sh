#!/bin/sh

# treat unset variables as an error
set -u

# if SERVER_URI is set, then use it instead
server_uri_opt=""
if [ ! -z $SERVER_URI ]; then
    server_uri_opt="--suri="
fi

/pki/gen_kc_pair.sh

python3 /app/headunit.py "${server_uri_opt}${SERVER_URI}"
