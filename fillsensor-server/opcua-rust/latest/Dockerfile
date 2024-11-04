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

# Copy app files
COPY ./app /app

# Build the app
RUN cd /app; \
    cargo build --release;

# startup is controlled by this script which depends on environment variables
COPY ./startup.sh /

# will be executed on startup
ENTRYPOINT [ "usr/bin/env" ]

# arguments passed to entrypoint, ensures that environment variables are set
CMD [ "/bin/sh", "/startup.sh" ]