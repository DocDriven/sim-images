FROM debian:bookworm-slim AS base

# set environment variables here with 'ENV VAR=value'
# this allows dynamic customization of container behavior


# update index and install packages if necessary with
RUN apt-get update && apt-get install -y \
    libssl-dev

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
          -DUA_ENABLE_SUBSCRIPTIONS=ON \
          -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON; \
    make && make install; \
    ldconfig /usr/local/bin

COPY fillsensor-server/open62541/latest/app /usr/src/app

RUN cd /usr/src/app; make; make install

FROM base AS runtime

COPY --from=builder /usr/local/bin /usr/local/bin

COPY --from=builder /usr/local/lib /usr/local/lib

# startup is controlled by this script which depends on environment variables
COPY fillsensor-server/open62541/latest/startup.sh /

# will be executed on startup
ENTRYPOINT [ "usr/bin/env" ]

# arguments passed to entrypoint, ensures that environment variables are set
CMD [ "/bin/sh", "/startup.sh" ]

