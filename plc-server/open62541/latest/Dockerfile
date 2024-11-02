FROM debian:bookworm-slim AS base

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior

# update index and install packages if necessary with
RUN apt-get update && apt-get install -y \
    openssl \
    libssl-dev \
    libsqlite3-dev

FROM base AS builder

RUN apt-get install -y \
    cmake \
    git \
    python3; \
    git clone https://github.com/open62541/open62541.git; \
    cd open62541; \
    mkdir build && cd build; \
    cmake .. \ 
      -DCMAKE_BUILD_TYPE=Release \
      -DUA_ENABLE_DA=ON \
      -DUA_ENABLE_DISCOVERY=ON \
      -DUA_ENABLE_ENCRYPTION=OPENSSL \
      -DUA_ENABLE_METHODCALLS=ON \
    make && make install; \
    ldconfig /usr/local/bin

COPY /app /usr/src/app

RUN cd /usr/src/app; make; make install

FROM base AS runtime

VOLUME /database

COPY --from=builder /usr/local/bin /usr/local/bin

COPY --from=builder /usr/local/lib /usr/local/lib

COPY /pki /pki

# startup is controlled by this script which depends on environment variables
COPY /startup.sh /

ENTRYPOINT [ "usr/bin/env" ]

CMD [ "/bin/sh", "/startup.sh" ]
