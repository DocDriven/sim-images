#!/bin/bash

KEY_LEN=2048
DAYS_VALID=365
CONFIG_FILE="/pki/ca.cnf"

if [ ! -f "$CONFIG_FILE" ]; then
  echo "Missing ca.cnf file at $CONFIG_FILE"
  exit 1
fi

# Generate the RSA key
openssl genrsa -out /pki/key.pem $KEY_LEN

# Convert the key to DER format
openssl pkey -inform PEM -outform DER -in /pki/key.pem -out /pki/key.der

# Generate the certificate
openssl req -x509 -new -key /pki/key.pem -out /pki/cert.der -outform DER \
        -config "$CONFIG_FILE" -days $DAYS_VALID \
        -addext "subjectAltName = URI:urn:opcua-server.local,IP:127.0.0.1,DNS:$HOSTNAME"
