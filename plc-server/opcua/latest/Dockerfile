FROM rust:slim-bookworm

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior

# update index and install packages if necessary with
RUN apt-get update && apt-get install -y \
    curl \
    gcc \
    libssl-dev \
    libsqlite3-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Mount volume
VOLUME /database

# Copy app files
COPY plc-server/opcua/latest/app /usr/src/app

# Build the app
RUN cd /usr/src/app; \
    cargo build --release;

# Copy the encryption-related files
COPY plc-server/opcua/latest/pki /pki

# startup is controlled by this script which depends on environment variables
COPY plc-server/opcua/latest/startup.sh /

# will be executed on startup
ENTRYPOINT [ "usr/bin/env" ]

# arguments passed to entrypoint, ensures that environment variables are set
CMD [ "/bin/sh", "/startup.sh" ]
