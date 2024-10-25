#!/bin/bash

KEY_LEN=2048
DAYS_VALID=365

if [ -f "/pki/ca.conf" ]; then
  CONFIG_FILE="/pki/ca.conf"
elif [ -f "/pki/ca.cnf" ]; then
  CONFIG_FILE="/pki/ca.cnf"
else
  echo "Missing ca.conf or ca.cnf file"
  exit 1
fi

openssl genrsa -out /pki/key.pem $KEY_LEN

openssl rsa -inform PEM -in /pki/key.pem -outform DER -out /pki/key.der

openssl req -x509 -new -key /pki/key.pem -out /pki/cert.der -outform der \
        -config $CONFIG_FILE -days $DAYS_VALID \
        -addext "subjectAltName = URI:urn:opcua-server.local, IP:127.0.0.1,DNS:$HOSTNAME"
