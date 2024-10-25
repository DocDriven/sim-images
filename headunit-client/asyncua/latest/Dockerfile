FROM debian:bookworm-slim AS base

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior

# address of the PLC OPC UA server
ENV SERVER_URI=

# update index and install packages if necessary with
RUN apt-get update && apt-get install -y \
    python3.11 \
    libssl-dev \
    libsqlite3-dev \
    sqlite3

FROM base AS builder

RUN apt-get install -y \
    build-essential \
    cmake \
    curl \
    python3-pip \
    libffi-dev \
    libjpeg-dev \
    pkg-config

RUN curl -L sh.rustup.rs | sh -s -- -y

ENV PATH="/root/.cargo/bin:${PATH}"

RUN pip install --break-system-packages \
    asyncua \
    pandas \
    dash

FROM base AS runtime

COPY --from=builder /usr/local/lib/python3.11/dist-packages /usr/local/lib/python3.11/dist-packages

COPY headunit-client/asyncua/latest/app /usr/src/app

COPY headunit-client/asyncua/latest/pki /pki

# startup is controlled by this script which depends on environment variables
COPY headunit-client/asyncua/latest/startup.sh /

# will be executed on startup
ENTRYPOINT [ "usr/bin/env" ]

# arguments passed to entrypoint, ensures that environment variables are set
CMD [ "/bin/sh", "/startup.sh" ]
